//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#include "kern_x6000p.hpp"
#include "kern_patcherplus.hpp"
#include "kern_patches.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_devinfo.hpp>

X6000P *X6000P::callback = nullptr;

void X6000P::init() {
	SYSLOG("x6000p", "Copyright 2022-2023 ChefKiss Inc. If you've paid for this, you've been scammed.");
	callback = this;

	lilu.onPatcherLoadForce(
		[](void *user, KernelPatcher &patcher) { static_cast<X6000P *>(user)->processPatcher(patcher); }, this);
	lilu.onKextLoadForce(
		nullptr, 0,
		[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
			static_cast<X6000P *>(user)->processKext(patcher, index, address, size);
		},
		this);
}

void X6000P::processPatcher(KernelPatcher &patcher) {
	auto *devInfo = DeviceInfo::create();
	if (devInfo) {
		devInfo->processSwitchOff();
        char name[256] = {0};
        for (size_t i = 0, ii = 0; i < devInfo->videoExternal.size(); i++) {
            auto *device = OSDynamicCast(IOPCIDevice, devInfo->videoExternal[i].video);
            if (device) {
                snprintf(name, arrsize(name), "GFX%zu", ii++);
                WIOKit::renameDevice(device, name);
                WIOKit::awaitPublishing(device);
            }
        }

        static uint8_t builtin[] = {0x00};
        this->GPU->setProperty("built-in", builtin, arrsize(builtin));
        this->deviceId = WIOKit::readPCIConfigValue(this->GPU, WIOKit::kIOPCIConfigDeviceID);
        this->pciRevision = WIOKit::readPCIConfigValue(NRed::callback->GPU, WIOKit::kIOPCIConfigRevisionID);
		
		DeviceInfo::deleter(devInfo);
	} else {
		SYSLOG("x6000p", "Failed to create DeviceInfo");
	}
}
