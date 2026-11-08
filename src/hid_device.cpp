#include "hid_device.h"

#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>

#include <cstring>
#include <vector>

namespace {

HANDLE openPath(const std::wstring& path, DWORD access) {
    return CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
}

bool readCaps(HANDLE handle, HIDP_CAPS& caps) {
    PHIDP_PREPARSED_DATA preparsed = nullptr;
    if (!HidD_GetPreparsedData(handle, &preparsed)) {
        return false;
    }
    bool ok = HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS;
    HidD_FreePreparsedData(preparsed);
    return ok;
}

}

HidDevice::~HidDevice() {
    close();
}

std::optional<std::wstring> HidDevice::findPath(uint16_t vendorId, uint16_t productId, uint16_t usagePage) {
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO devices = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devices == INVALID_HANDLE_VALUE) {
        return std::nullopt;
    }

    std::optional<std::wstring> result;
    SP_DEVICE_INTERFACE_DATA interfaceData{};
    interfaceData.cbSize = sizeof(interfaceData);

    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(devices, nullptr, &hidGuid, index, &interfaceData); ++index) {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(devices, &interfaceData, nullptr, 0, &required, nullptr);
        if (required == 0) {
            continue;
        }

        std::vector<uint8_t> buffer(required);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devices, &interfaceData, detail, required, nullptr, nullptr)) {
            continue;
        }

        std::wstring path = detail->DevicePath;
        HANDLE handle = openPath(path, 0);
        if (handle == INVALID_HANDLE_VALUE) {
            continue;
        }

        HIDD_ATTRIBUTES attributes{};
        attributes.Size = sizeof(attributes);
        HIDP_CAPS caps{};
        bool match = HidD_GetAttributes(handle, &attributes) && attributes.VendorID == vendorId &&
                     attributes.ProductID == productId && readCaps(handle, caps) && caps.UsagePage == usagePage;
        CloseHandle(handle);

        if (match) {
            result = path;
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(devices);
    return result;
}

bool HidDevice::open(const std::wstring& path) {
    close();
    HANDLE handle = openPath(path, GENERIC_READ | GENERIC_WRITE);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    HIDD_ATTRIBUTES attributes{};
    attributes.Size = sizeof(attributes);
    HIDP_CAPS caps{};
    if (!HidD_GetAttributes(handle, &attributes) || !readCaps(handle, caps)) {
        CloseHandle(handle);
        return false;
    }

    handle_ = handle;
    releaseNumber_ = attributes.VersionNumber;
    featureLength_ = caps.FeatureReportByteLength;
    return true;
}

void HidDevice::close() {
    if (handle_) {
        CloseHandle(static_cast<HANDLE>(handle_));
        handle_ = nullptr;
    }
}

bool HidDevice::isOpen() const {
    return handle_ != nullptr;
}

bool HidDevice::setFeature(const uint8_t* data, size_t length) {
    if (!handle_) {
        return false;
    }
    std::vector<uint8_t> buffer(data, data + length);
    if (HidD_SetFeature(static_cast<HANDLE>(handle_), buffer.data(), static_cast<ULONG>(buffer.size()))) {
        return true;
    }
    if (featureLength_ > length) {
        buffer.resize(featureLength_, 0);
        return HidD_SetFeature(static_cast<HANDLE>(handle_), buffer.data(), static_cast<ULONG>(buffer.size())) != FALSE;
    }
    return false;
}

bool HidDevice::getFeature(uint8_t* data, size_t length) {
    if (!handle_) {
        return false;
    }
    if (HidD_GetFeature(static_cast<HANDLE>(handle_), data, static_cast<ULONG>(length))) {
        return true;
    }
    if (featureLength_ > length) {
        std::vector<uint8_t> buffer(featureLength_, 0);
        buffer[0] = data[0];
        if (HidD_GetFeature(static_cast<HANDLE>(handle_), buffer.data(), static_cast<ULONG>(buffer.size()))) {
            std::memcpy(data, buffer.data(), length);
            return true;
        }
    }
    return false;
}
