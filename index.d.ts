export interface Device {
  /** device id */
  id: string;
  /** parent device id, used for composition device */
  parent: string;
  /** device name */
  name: string;
  /** device path, used for write data to device in win32 */
  path: string;
  /** vendor id of device */
  vendorId: string;
  /** product id of device */
  productId: string;
  /** serial number of device */
  serialNumber: string;
}

export function find(): Device[];

export function writeData(
  devicePath: string,
  data: string,
  shouldRead: boolean = false
): string;

export function startMonitoring(): void;

export function stopMonitoring(): void;

export function on(evnet: "add", callback: (devicePath: string) => void): void;
export function on(
  evnet: "remove",
  callback: (devicePath: string) => void
): void;
