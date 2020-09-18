if (process.platform !== "win32") {
  console.error("`node-usb-printer` only support `Windows` for now.");
  process.exit(1);
}

const addon = require("bindings")("addon");
const { EventEmitter2 } = require("eventemitter2");

const usbPrinterManager = new EventEmitter2({
  wildcard: true,
  delimiter: ":",
  maxListeners: 1000,
});

function initEvents() {
  addon.registerAddedCallback((devicePath) => {
    usbPrinterManager.emit("add", devicePath);
  });

  addon.registerRemovedCallback((devicePath) => {
    usbPrinterManager.emit("remove", devicePath);
  });
}

usbPrinterManager.find = () => {
  const deviceList = addon.find();
  return deviceList.map((device) => {
    let [_usb, vidpid, serialNumber] = device.id.split("\\");
    const [vendorId, productId, interfaceNumber] = vidpid
      .split("&")
      .map((part) => {
        const [_prefix, value] = part.split("_");
        if (value) return parseInt(value, 16);
        return part;
      });
    // vidpid like USB\\VID_6868&PID_0500&MI_00\\6&2853A1DE&0&0000
    if (interfaceNumber !== undefined) {
      // parent like USB\\VID_6868&PID_0500\\6A69A2BD1237
      serialNumber = device.parent.split("\\").pop();
    }
    return {
      ...device,
      vendorId,
      productId,
      serialNumber,
    };
  });
};
usbPrinterManager.writeData = addon.writeData;

let isStartedMonitoring = false;

usbPrinterManager.startMonitoring = () => {
  if (isStartedMonitoring) return;
  isStartedMonitoring = true;

  // in order to prevent process from blocking, initilize events when start monitoring
  initEvents();

  addon.startMonitoring();
};

usbPrinterManager.stopMonitoring = () => {
  if (!isStartedMonitoring) return;
  isStartedMonitoring = false;

  addon.stopMonitoring();
};

module.exports = usbPrinterManager;
