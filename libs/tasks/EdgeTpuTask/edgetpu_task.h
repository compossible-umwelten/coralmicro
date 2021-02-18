#ifndef _LIBS_TASKS_EDGETPUTASK_EDGETPU_TASK_H_
#define _LIBS_TASKS_EDGETPUTASK_EDGETPU_TASK_H_

#include "libs/nxp/rt1176-sdk/usb_host_config.h"
#include "third_party/nxp/rt1176-sdk/middleware/usb/host/usb_host.h"
#include "third_party/nxp/rt1176-sdk/middleware/usb/host/usb_host_hci.h"
#include "third_party/nxp/rt1176-sdk/middleware/usb/include/usb.h"

namespace valiant {

constexpr int kEdgeTpuVid = 0x18d1;
constexpr int kEdgeTpuPid = 0x9302;

enum edgetpu_state {
    EDGETPU_STATE_UNATTACHED = 0,
    EDGETPU_STATE_ATTACHED,
    EDGETPU_STATE_SET_INTERFACE,
    EDGETPU_STATE_GET_STATUS,
    EDGETPU_STATE_CONNECTED,
    EDGETPU_STATE_ERROR,
};

class EdgeTpuTask {
  public:
    EdgeTpuTask();
    EdgeTpuTask(const EdgeTpuTask&) = delete;
    EdgeTpuTask& operator=(const EdgeTpuTask&) = delete;

    void EdgeTpuTaskFn();
    static EdgeTpuTask* GetSingleton() {
        static EdgeTpuTask task;
        return &task;
    }

    void SetDeviceHandle(usb_device_handle handle) {
      device_handle_ = handle;
    }
    usb_device_handle device_handle() {
      return device_handle_;
    }

    void SetInterfaceHandle(usb_host_interface_handle handle) {
      interface_handle_ = handle;
    }
    usb_host_interface_handle interface_handle() {
      return interface_handle_;
    }

    void SetClassHandle(usb_host_class_handle handle) {
      class_handle_ = handle;
    }
    usb_host_class_handle class_handle() {
      return class_handle_;
    }
  private:
    void SetNextState(enum edgetpu_state next_state);
    static void SetInterfaceCallback(void *param, uint8_t *data, uint32_t data_length, usb_status_t status);
    static void GetStatusCallback(void *param, uint8_t *data, uint32_t data_length, usb_status_t status);
    usb_status_t USBHostEvent(
            usb_host_handle host_handle,
            usb_device_handle device_handle,
            usb_host_configuration_handle config_handle,
            uint32_t event_code);

    usb_device_handle device_handle_;
    usb_host_interface_handle interface_handle_;
    usb_host_class_handle class_handle_;
    uint8_t status_;

    OSA_MSGQ_HANDLE_DEFINE(message_queue_, 1, sizeof(uint32_t));
};
}  // namespace valiant

#endif  // _LIBS_TASKS_EDGETPUTASK_EDGETPU_TASK_H_