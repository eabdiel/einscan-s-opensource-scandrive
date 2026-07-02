// EinScan-S legacy native CLI bridge for the deprecated SHINING 3D SDK.
//
// Python calls this executable with commands like:
//   einscan_legacy_bridge.exe detect
//   einscan_legacy_bridge.exe calibrate
//   einscan_legacy_bridge.exe scan --quality medium --texture 0 --format obj --out output/einscan_scan.obj
//
// Build target: Windows x64. The legacy SDK was built around VS2013 / vc120 DLLs.

#include <windows.h>
#include <direct.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>

#include "sn3d_pro_sdk.h"

#ifndef SN3D_RET_NOERROR
#define SN3D_RET_NOERROR 0
#endif

static void* g_handle = NULL;
static volatile LONG g_event_count = 0;
static volatile LONG g_mesh_ready = 0;
static volatile LONG g_whole_cloud_ready = 0;
static volatile LONG g_calibration_finished = 0;

static std::string arg_value(const std::vector<std::string>& args, const std::string& key, const std::string& fallback) {
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == key) return args[i + 1];
    }
    return fallback;
}

static int arg_int(const std::vector<std::string>& args, const std::string& key, int fallback) {
    std::string v = arg_value(args, key, "");
    if (v.empty()) return fallback;
    return atoi(v.c_str());
}

static double quality_to_resolution(const std::string& quality) {
    if (quality == "high") return 0.5;
    if (quality == "low") return 2.0;
    return 1.0; // medium
}

static int mesh_quality(const std::string& quality) {
    if (quality == "high") return SN3D_MESH_HIGHT;
    if (quality == "low") return SN3D_MESH_LOW;
    return SN3D_MESH_MIDDLE;
}

static void ensure_parent_dir(const std::string& out) {
    size_t slash = out.find_last_of("/\\");
    if (slash == std::string::npos) return;
    std::string dir = out.substr(0, slash);
    if (!dir.empty()) _mkdir(dir.c_str());
}

static const char* ret_name(int code) {
    switch (code) {
        case SN3D_RET_NOERROR: return "SN3D_RET_NOERROR";
        case SN3D_RET_PARAM_ERROR: return "SN3D_RET_PARAM_ERROR";
        case SN3D_RET_ORDER_ERROR: return "SN3D_RET_ORDER_ERROR";
        case SN3D_RET_TIME_OUT_ERROR: return "SN3D_RET_TIME_OUT_ERROR";
        case SN3D_RET_NOT_SUPPORT_ERROR: return "SN3D_RET_NOT_SUPPORT_ERROR";
        case SN3D_RET_NO_DEVICE_ERROR: return "SN3D_RET_NO_DEVICE_ERROR";
        case SN3D_RET_DEVICE_LICENSE_ERROR: return "SN3D_RET_DEVICE_LICENSE_ERROR";
        case SN3D_RET_GPU_ERROR: return "SN3D_RET_GPU_ERROR";
        case SN3D_RET_INNER_ERROR: return "SN3D_RET_INNER_ERROR";
        case SN3D_RET_NOT_CALIBRATE_ERROR: return "SN3D_RET_NOT_CALIBRATE_ERROR";
        case SN3D_RET_LOST_CONFIG_FILE_ERROR: return "SN3D_RET_LOST_CONFIG_FILE_ERROR";
        case SN3D_RET_NO_DATA_ERROR: return "SN3D_RET_NO_DATA_ERROR";
        case SN3D_RET_LOST_CALIBRATE_FILE_ERROR: return "SN3D_RET_LOST_CALIBRATE_FILE_ERROR";
        case SN3D_RET_NO_GLOBAL_MARK_POINT_PARAM_ERROR: return "SN3D_RET_NO_GLOBAL_MARK_POINT_PARAM_ERROR";
        default: return "SN3D_UNKNOWN_RETURN";
    }
}

static void print_ret(const std::string& action, int rc) {
    std::cout << action << " -> " << rc << " (" << ret_name(rc) << ")" << std::endl;
}

static void CALLBACK scan_callback(void* handle, int event_type, int event_sub_type, void* data, int data_len) {
    InterlockedIncrement(&g_event_count);
    std::cout << "EVENT type=" << event_type << " sub=" << event_sub_type << " len=" << data_len << std::endl;

    if (event_type == SN3D_DISTANCE_INDECATE && data) {
        int distance = *((int*)data);
        std::cout << "DISTANCE " << distance << std::endl;
    }
    else if (event_type == SN3D_CALIBRATION_STATE_DATA && data) {
        SN3D_STATE_CALIBRATE* st = (SN3D_STATE_CALIBRATE*)data;
        std::cout << "CALIBRATE state=" << st->current_calibrate
                  << " group=" << st->current_group
                  << " snap=" << st->snap_state
                  << " compute=" << st->compute_state
                  << " distance=" << st->distance_indicate << std::endl;
        if (st->current_calibrate == SN3D_CALIBRATE_STATE_EXIT || st->compute_state == SN3D_CALIBRATE_COMPUTE_SUCCESS) {
            InterlockedExchange(&g_calibration_finished, 1);
        }
    }
    else if (event_type == SN3D_WHOLE_SCAN_POINT_CLOUD_READY || event_type == SN3D_FINISH_COMPLETED) {
        InterlockedExchange(&g_whole_cloud_ready, 1);
    }
    else if (event_type == SN3D_MESH_DATA_READY) {
        InterlockedExchange(&g_mesh_ready, 1);
    }
}

static std::wstring widen(const std::string& s) {
    if (s.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (size_needed <= 0) {
        size_needed = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, 0);
        if (size_needed <= 0) return L"";
        std::wstring w(size_needed, 0);
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], size_needed);
        if (!w.empty() && w.back() == L'\0') w.pop_back();
        return w;
    }
    std::wstring w(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], size_needed);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

static std::string exe_dir() {
    char buffer[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return ".";
    std::string path(buffer);
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) return ".";
    return path.substr(0, slash);
}

static void set_sdk_workdir(const std::vector<std::string>& args) {
    std::string cwd = arg_value(args, "--workdir", "");
    if (cwd.empty()) {
        const char* env = std::getenv("EINSCAN_SDK_WORKDIR");
        if (env && env[0]) cwd = env;
    }
    if (cwd.empty()) cwd = exe_dir();
    SetCurrentDirectoryA(cwd.c_str());
    char actual[MAX_PATH] = {0};
    GetCurrentDirectoryA(MAX_PATH, actual);
    std::cout << "SDK working directory=" << actual << std::endl;
}

static void init_data(SN3D_INIT_DATA& data, const std::string& device_type, const std::string& config_path = "") {
    ZeroMemory(&data, sizeof(SN3D_INIT_DATA));
    data.device_type = const_cast<char*>(device_type.c_str());
    data.config_path[0] = L'\0';
    std::cout << "Using SDK device_type=" << data.device_type << std::endl;

    std::string chosen = config_path;
    if (chosen.empty()) {
        const char* env_path = std::getenv("EINSCAN_CONFIG_PATH");
        if (env_path && env_path[0]) chosen = env_path;
    }

    if (!chosen.empty()) {
        std::wstring w = widen(chosen);
        if (!w.empty()) {
            wcsncpy_s(data.config_path, SN3D_MAX_PATH, w.c_str(), _TRUNCATE);
            std::wcout << L"Using SDK config_path=" << data.config_path << std::endl;
        }
    } else {
        std::cout << "Using SDK config_path=<blank like official demo>" << std::endl;
    }
}

static int open_handle(int init_type, const std::string& config_path = "", const std::string& device_type = "EinScan-Pro") {
    if (g_handle) {
        sn3d_close(g_handle);
        g_handle = NULL;
    }
    SN3D_INIT_DATA data;
    init_data(data, device_type, config_path);
    int rc = sn3d_Initialize(init_type, &data, g_handle);
    print_ret("sn3d_Initialize", rc);
    if (rc != SN3D_RET_NOERROR) return rc;

    sn3d_callback callback_fn = reinterpret_cast<sn3d_callback>(&scan_callback);
    rc = sn3d_regist_callback(g_handle, callback_fn, nullptr);
    print_ret("sn3d_regist_callback", rc);
    return rc;
}

static void close_handle() {
    if (g_handle) {
        int rc = sn3d_close(g_handle);
        print_ret("sn3d_close", rc);
        g_handle = NULL;
    }
}

static bool wait_for_flag(volatile LONG* flag, int seconds, const std::string& label) {
    for (int i = 0; i < seconds * 10; ++i) {
        if (*flag) return true;
        Sleep(100);
    }
    std::cout << "TIMEOUT waiting for " << label << std::endl;
    return false;
}

static int export_cloud_points(const std::string& out, const std::string& format) {
    SN3D_CLOUD_POINT cloud;
    ZeroMemory(&cloud, sizeof(cloud));

    int rc = sn3d_get_current_scan_whole_point_cloud(g_handle, cloud);
    print_ret("sn3d_get_current_scan_whole_point_cloud count", rc);
    if (rc != SN3D_RET_NOERROR && rc <= 0) return rc;

    int count = cloud.vertex_count;
    if (count <= 0) {
        // Some SDK builds return count as the return value on the first call.
        count = rc > 0 ? rc : 0;
    }
    if (count <= 0) {
        std::cerr << "No point cloud vertices returned." << std::endl;
        return SN3D_RET_NO_DATA_ERROR;
    }

    std::vector<SN3D_POINT_DATA> vertices(count);
    std::vector<SN3D_POINT_DATA> normals(count);
    ZeroMemory(&cloud, sizeof(cloud));
    cloud.vertex_count = count;
    cloud.vertex_data = &vertices[0];
    cloud.norma_count = count;
    cloud.norma_data = &normals[0];

    rc = sn3d_get_current_scan_whole_point_cloud(g_handle, cloud);
    print_ret("sn3d_get_current_scan_whole_point_cloud data", rc);
    if (rc != SN3D_RET_NOERROR) return rc;

    ensure_parent_dir(out);
    std::ofstream f(out.c_str());
    if (!f) {
        std::cerr << "Could not write output: " << out << std::endl;
        return SN3D_RET_PARAM_ERROR;
    }
    int final_count = cloud.vertex_count > 0 ? cloud.vertex_count : count;
    if (format == "ply") {
        f << "ply\nformat ascii 1.0\nelement vertex " << final_count << "\n";
        f << "property float x\nproperty float y\nproperty float z\nend_header\n";
        for (int i = 0; i < final_count; ++i) {
            f << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << "\n";
        }
    } else {
        f << "# Exported by EinScan-S Python Bridge\n";
        for (int i = 0; i < final_count; ++i) {
            f << "v " << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << "\n";
        }
    }
    f.close();
    std::cout << "Exported point cloud: " << out << " vertices=" << final_count << std::endl;
    return SN3D_RET_NOERROR;
}


static const char* init_type_name(int init_type) {
    switch (init_type) {
        case SN3D_INIT_CALIBRATE: return "SN3D_INIT_CALIBRATE";
        case SN3D_INIT_RAPIDSCAN: return "SN3D_INIT_RAPIDSCAN";
        case SN3D_INIT_HD_SCAN: return "SN3D_INIT_HD_SCAN";
        case SN3D_INIT_FIX_SCAN: return "SN3D_INIT_FIX_SCAN";
        default: return "SN3D_INIT_UNKNOWN";
    }
}

static int probe_init_modes(const std::vector<std::string>& args) {
    std::cout << "EinScan-S SDK init-mode probe" << std::endl;
    std::cout << "This checks whether any SDK init path can open on this GPU/driver setup." << std::endl;
    std::cout << "If all modes return -8, the SDK is hard-blocked by GPU/CUDA requirements." << std::endl;

    int modes[] = {
        SN3D_INIT_CALIBRATE,
        SN3D_INIT_RAPIDSCAN,
        SN3D_INIT_HD_SCAN,
        SN3D_INIT_FIX_SCAN
    };

    std::string config_path = arg_value(args, "--config", "");
    std::string requested_device = arg_value(args, "--device", "");
    std::vector<std::string> devices;
    if (!requested_device.empty()) {
        devices.push_back(requested_device);
    } else {
        devices.push_back("EinScan-Pro");
        devices.push_back("EinScan-Plus");
    }
    set_sdk_workdir(args);
    bool any_ok = false;
    for (size_t d = 0; d < devices.size(); ++d) {
        for (int i = 0; i < 4; ++i) {
            int mode = modes[i];
            std::cout << "\n--- " << devices[d] << " / " << init_type_name(mode) << " (" << mode << ") ---" << std::endl;

            if (g_handle) {
                sn3d_close(g_handle);
                g_handle = NULL;
            }

            SN3D_INIT_DATA data;
            init_data(data, devices[d], config_path);
            int rc = sn3d_Initialize(mode, &data, g_handle);
            print_ret("sn3d_Initialize", rc);

            if (rc == SN3D_RET_NOERROR && g_handle) {
                any_ok = true;
                sn3d_callback callback_fn = reinterpret_cast<sn3d_callback>(&scan_callback);
                int cb = sn3d_regist_callback(g_handle, callback_fn, nullptr);
                print_ret("sn3d_regist_callback", cb);
                close_handle();
            } else {
                if (g_handle) {
                    close_handle();
                }
            }
        }
    }

    std::cout << "\nProbe result: " << (any_ok ? "AT_LEAST_ONE_INIT_MODE_WORKED" : "NO_INIT_MODE_WORKED") << std::endl;
    return any_ok ? 0 : 1;
}

static int detect_scanner(const std::vector<std::string>& args) {
    std::cout << "Detecting EinScan-S via deprecated SDK..." << std::endl;
    std::string config_path = arg_value(args, "--config", "");
    std::string device_type = arg_value(args, "--device", "EinScan-Pro");
    set_sdk_workdir(args);
    int rc = open_handle(SN3D_INIT_RAPIDSCAN, config_path, device_type);
    close_handle();
    return rc == SN3D_RET_NOERROR ? 0 : rc;
}

static int calibrate_scanner(const std::vector<std::string>& args) {
    int wait_seconds = arg_int(args, "--wait", 120);
    std::string config_path = arg_value(args, "--config", "");
    std::string device_type = arg_value(args, "--device", "EinScan-Pro");
    set_sdk_workdir(args);
    int rc = open_handle(SN3D_INIT_CALIBRATE, config_path, device_type);
    if (rc != SN3D_RET_NOERROR) { close_handle(); return rc; }

    rc = sn3d_start_calibrate(g_handle);
    print_ret("sn3d_start_calibrate", rc);
    if (rc == SN3D_RET_NOERROR) wait_for_flag(&g_calibration_finished, wait_seconds, "calibration finished");
    close_handle();
    return rc == SN3D_RET_NOERROR ? 0 : rc;
}

static int scan_and_export(const std::vector<std::string>& args) {
    std::string out = arg_value(args, "--out", "output/einscan_scan.obj");
    std::string quality = arg_value(args, "--quality", "medium");
    std::string texture = arg_value(args, "--texture", "0");
    std::string format = arg_value(args, "--format", "obj");
    int duration = arg_int(args, "--duration", 15);
    int align = arg_int(args, "--align", SN3D_ALIGN_MODE_FEATURE);
    int init_type = arg_int(args, "--init", SN3D_INIT_RAPIDSCAN);
    std::string config_path = arg_value(args, "--config", "");
    std::string device_type = arg_value(args, "--device", "EinScan-Pro");
    set_sdk_workdir(args);

    int rc = open_handle(init_type, config_path, device_type);
    if (rc != SN3D_RET_NOERROR) { close_handle(); return rc; }

    SN3D_SCAN_PARAM param;
    ZeroMemory(&param, sizeof(param));
    param.resolution = quality_to_resolution(quality);
    param.flag_texture = (texture == "1" || texture == "true" || texture == "yes") ? 1 : 0;
    param.align_mode = align;

    rc = sn3d_set_scan_param(g_handle, &param);
    print_ret("sn3d_set_scan_param", rc);
    if (rc != SN3D_RET_NOERROR) { close_handle(); return rc; }

    std::cout << "Starting scan. Move/turn the object now. Duration=" << duration << " seconds" << std::endl;
    rc = sn3d_start_scan(g_handle);
    print_ret("sn3d_start_scan", rc);
    if (rc != SN3D_RET_NOERROR) { close_handle(); return rc; }

    Sleep(duration * 1000);

    rc = sn3d_finish_scan(g_handle);
    print_ret("sn3d_finish_scan", rc);
    if (rc != SN3D_RET_NOERROR) { close_handle(); return rc; }

    wait_for_flag(&g_whole_cloud_ready, 10, "whole point cloud ready");
    rc = export_cloud_points(out, format);
    close_handle();
    return rc == SN3D_RET_NOERROR ? 0 : rc;
}

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.push_back(argv[i]);
    if (args.empty()) {
        std::cerr << "Usage: einscan_legacy_bridge.exe detect|probe|calibrate|scan [--device EinScan-Pro|EinScan-Plus] [--config path] [--workdir path] [--duration 15] [--quality high|medium|low] [--texture 0|1] [--out file.obj]" << std::endl;
        return 2;
    }

    if (args[0] == "detect") return detect_scanner(args);
    if (args[0] == "probe") return probe_init_modes(args);
    if (args[0] == "calibrate") return calibrate_scanner(args);
    if (args[0] == "scan") return scan_and_export(args);

    std::cerr << "Unknown command: " << args[0] << std::endl;
    return 2;
}
