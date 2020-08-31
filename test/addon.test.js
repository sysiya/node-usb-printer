const usbPrinterManager = require("../index");

const MANUAL_INTERACTION_TIMEOUT = 10000;

function once(eventName) {
  return new Promise(function (resolve) {
    usbPrinterManager.on(eventName, function (device) {
      resolve(device);
    });
  });
}

describe("node-usb-printer", () => {
  describe("API", () => {
    beforeAll(() => {
      usbPrinterManager.startMonitoring();
    });

    afterAll(() => {
      usbPrinterManager.stopMonitoring();
    });

    describe(".find", () => {
      it("should find some usb printers (after connected)", () => {
        const deviceList = usbPrinterManager.find();
        expect(Array.isArray(deviceList)).toBe(true);
      });

      it("should return the right device object shape (after connected)", () => {
        const deviceList = usbPrinterManager.find();
        deviceList.forEach((device) => {
          expect(device).toHaveProperty("id");
          expect(device).toHaveProperty("parent");
          expect(device).toHaveProperty("name");
          expect(device).toHaveProperty("path");
          expect(device).toHaveProperty("vendorId");
          expect(device).toHaveProperty("productId");
          expect(device).toHaveProperty("serialNumber");
        });
      });
    });

    describe(".writeData", () => {
      it("shoud return query response for GP-80 (after connected)", () => {
        const deviceList = usbPrinterManager.find();
        deviceList.forEach((device) => {
          if (device.vendorId === 0x6868 && device.productId === 0x0500) {
            const response = usbPrinterManager.writeData(
              device.path,
              `\x1B\x21\x3F\r\n`,
              true
            );
            expect(response).not.toBe(undefined);
          }
        });
      });
    });

    describe("Event", () => {
      it(
        "should listen to device added (first insert usb prniter)",
        (done) => {
          once("add")
            .then(function (devicePath) {
              expect(devicePath).toEqual(expect.any(String));
            })
            .then(done)
            .catch(done.fail);
        },
        MANUAL_INTERACTION_TIMEOUT
      );

      it(
        "should listen to device removed (then remove usb printer)",
        (done) => {
          once("remove")
            .then(function (devicePath) {
              expect(devicePath).toEqual(expect.any(String));
            })
            .then(done)
            .catch(done.fail);
        },
        MANUAL_INTERACTION_TIMEOUT
      );
    });
  });
});
