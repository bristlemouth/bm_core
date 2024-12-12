from serial_helper import SerialHelper
import os
import struct
from collections import namedtuple
from typing import Dict, Any
import crcmod
from pathlib import Path
from func_timeout import func_timeout, FunctionTimedOut
from util import print_progress_bar
import pytest


class TestDFU:

    SUCCESSFUL_TIMEOUT_S = 60
    BYTES_TO_SEND = 16

    def __get_version_from_bin(self, file: str, offset: int = 0) -> Dict[str, Any]:
        """Get versioning information from file

        Obtains the versioning information from the binary application,
        this includes the sha and version. This reads in file byte by
        byte attempts to find the header from when the binary was
        generated.

        Args:
            file (str): Binary application file to search in.
            offset (int): Offset to begin searching file in.

        Returns:
            Dict[str, Any]: SHA and version number found in header.
        """
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
        """After the DFU wait until it succeeds or fails

        This is to be ran after DFU update to see if the DFU succeeds
        or fails

        Args:
            ser (SerialHelper): Serial helper instance.

        Returns:
            bool: True if successful, False if unsuccessful
        """
        ret = False
        while True:
            read = ser.read_line()
            if "Update successful" in read:
                ret = True
                break
            if "Update failed" in read:
                break
        return ret

    def test_dfu(self, ser: SerialHelper, file: str, node_id: str):
        """Test DFU functionality

        Runs through a DFU update over the serial console. Will send
        bytes to the device in the order of BYTES_TO_SEND. A Node ID
        and a proper DFU binary file path must be passed into the
        command line.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
            file (str): File name passed in from starting the pytests
                        (see: conftest.py).
            node_id (str): Node ID passed in from starting the pytests
                           (see: conftest.py).
        """
        if file is not None and node_id is not None:
            file = os.path.abspath(file)
            size = os.path.getsize(file)
            data = Path(file).read_bytes()

            kermit = crcmod.predefined.mkCrcFun("kermit")
            crc = kermit(data)
            fw_ver = self.__get_version_from_bin(file)
            assert fw_ver is not None
            major = int(fw_ver["version"].split(".")[0])
            minor = int(fw_ver["version"].split(".")[1])
            sha = int(fw_ver["sha"], 16)

            # Start dfu update
            tx = (
                "bm dfu start "
                + str(node_id)
                + " "
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

            # Obtain the number of times a message must be sent
            _range = len(data) / self.BYTES_TO_SEND
            # iF there is a bit extra data, round up
            if not _range.is_integer():
                _range += 1
            _range = int(_range)

            for i in range(_range):
                # Build the transmit message
                tx = "bm dfu byte"
                offset = i * self.BYTES_TO_SEND
                max_bytes = (
                    self.BYTES_TO_SEND
                    if size - offset > self.BYTES_TO_SEND
                    else size - offset
                )
                for j in range(max_bytes):
                    tx += " " + str(data[offset + j])
                tx += "\n"

                # Send the built message
                ser.flush()
                ser.transmit_str(tx)
                read = ser.read_until("DFU Byte Added\n")
                if "Update failed" in read:
                    break

                # Add a pretty progress bar
                count += self.BYTES_TO_SEND
                print_progress_bar("DFU Update Progress", count, size)

            # Finish the transmission with any leftover data in buffer
            ser.transmit_str("bm dfu finish\n")
            read = ser.read_until("DFU Finish\n")

            # Determine if DFU passed
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
            pytest.skip(
                "\nSkipping DFU test as file and node ID were not passed"
                " in to command line..."
            )
