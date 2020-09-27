#! /usr/bin/env nix-shell
#! nix-shell -i bash -p coreutils
set -eou pipefail

[[ $# -ne 1 ]] && echo "usage: ${0} <target to flash>" && exit 1
[[ ! -e ${1} ]] && echo "Target ${1} does not exist." && exit 2
[[ ! -w ${1} ]] && echo "Target ${1} is not writable. Do you have permissions?" && exit 2

echo "Building image..."
image=$(./mmc-image.nix)
echo "Flashing ${1}..."
dd if=${image} of=${1} bs=64k conv=fdatasync

# TODO: check UID of SD card at start of script, ensure it is the same when starting dd.
# TODO: try pv out again
