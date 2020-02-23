#!/usr/bin/env bash

set -e

DISK=$1
DATA_DIR=$2
LOOP_DEV=$(sudo losetup -f)
MOUNT_POINT=mldisk
SIZE=10M

echo "Creating $SIZE file storage $DISK."

# Create a partition.
qemu-img create -f raw $DISK $SIZE
fdisk $DISK <<EOF
n




w
EOF

# Create a file system. 2048 * 512 = 1048576.
sudo losetup -o 1048576 $LOOP_DEV $DISK
sudo mkfs.ext4 $LOOP_DEV

# Write an empty file to the file system.
mkdir $MOUNT_POINT
sudo mount $LOOP_DEV $MOUNT_POINT
sudo sh -c "cat <<EOF > $MOUNT_POINT/README
+---------------------------------------------------+
|   __  __                   _ _                    |
|  |  \/  |                 | (_)                   |
|  | \  / | ___  _ __   ___ | |_ _ __  _   ___  __  |
|  | |\/| |/ _ \| '_ \ / _ \| | | '_ \| | | \ \/ /  |
|  | |  | | (_) | | | | (_) | | | | | | |_| |>  <   |
|  |_|  |_|\___/|_| |_|\___/|_|_|_| |_|\__,_/_/\_\  |
|                                                   |
+---------------------------------------------------+
EOF"

echo "Data dir: $DATA_DIR"

if [ -e "$DATA_DIR" ] ; then
    echo "Copying files from $DATA_DIR to disk."
    sudo cp -r $DATA_DIR $MOUNT_POINT
else
    echo "No files to copy. "
fi

sudo umount $MOUNT_POINT
rm -rf $MOUNT_POINT

sudo losetup -d $LOOP_DEV
