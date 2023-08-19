import shutil

def copy_files():
    source_file = r"C:\Projects\DrizzlingBytes\Examples\target_firmware\Release\target_firmware.txt"
    target_file = r"C:\Projects\DrizzlingBytes\Examples\host_firmware\data\target_firmware.txt"

    shutil.copy(source_file, target_file)

copy_files()
