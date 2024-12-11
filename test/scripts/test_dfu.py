from serial_helper import SerialHelper
import os
import struct
from collections import namedtuple
from typing import Dict, Any
import crcmod
from pathlib import Path
from func_timeout import func_timeout, FunctionTimedOut
import sys


class TestDFU:

    SUCCESSFUL_TIMEOUT_S = 60

    def __get_version_from_bin(self, file: str, offset: int = 0) -> Dict[str, Any]:
        VersionHeader = namedtuple("VersionHeader", "magic gitSHA maj min")
        magic = 0xDF7F9AFDEC06627C
        header_format = "QIBB"
        header_len = struct.calcsize(header_format)
        version = None
        with open(file, "rb") as f:
            data = f.read()

            for offset in range(offset, len(data) - header_len):
                header = VersionHeader._make(
                    struct.unpack_from(header_format, data, offset=offset)
                )

                if magic == header.magic:
                    version = {
                        "sha": f"{header.gitSHA:08X}",
                        "version": f"{header.maj}.{header.min}",
                    }
                    break
        return version

    def __wait_until_update_success(self, ser: SerialHelper) -> bool:
        ret = False
        while True:
            read = ser.read_line()
            if "Update successful" in read:
                ret = True
                break
            if "Update failed" in read:
                break
        return ret

    def test_dfu_perform(self, ser: SerialHelper, file: str):
        if file is not None:
            # try:
            file = os.path.abspath(file)
            size = os.path.getsize(file)
            data = Path(file).read_bytes()

            kermit = crcmod.predefined.mkCrcFun("kermit")
            crc = kermit(data)
            fw_ver = self.__get_version_from_bin(file)
            major = int(fw_ver["version"].split(".")[0])
            minor = int(fw_ver["version"].split(".")[1])
            sha = int(fw_ver["sha"], 16)
            tx = (
                "bm dfu start 0x8b718c26aea126dc "
                + str(size)
                + " "
                + str(crc)
                + " "
                + str(major)
                + " "
                + str(minor)
                + " "
                + str(sha)
                + "\n"
            )
            ser.transmit_str(tx)
            assert "Received ACK\n" in ser.read_until("Received ACK\n")
            count = 0
            print("\n")
            for byte in data:
                ser.flush()
                ser.transmit_str("bm dfu byte " + str(byte) + "\n")
                read = ser.read_until("DFU Byte Added\n")
                if "Update failed" in read:
                    break

                # Add a pretty progress bar
                count += 1
                progress = int((count / size) * 100.0)
                sys.stdout.write(
                    f"\rDFU Update Progress: [{'#' * progress}{' ' * (100 - progress)}]"
                    f"{progress}%"
                )
                sys.stdout.flush()
            ser.transmit_str("bm dfu finish\n")
            read = ser.read_until("DFU Finish\n")
            try:
                assert (
                    func_timeout(
                        self.SUCCESSFUL_TIMEOUT_S,
                        self.__wait_until_update_success,
                        args=(ser,),
                    )
                    is True
                )
            except FunctionTimedOut:
                print("Update was not successful...")
                assert 0

        else:
            print("\nSkipping DFU test as file was not passed in...")
        pass
