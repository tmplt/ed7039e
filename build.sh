#! /usr/bin/env nix-shell
#! nix-shell -i bash -p coreutils pv zstd bc sudo
set -eou pipefail

[[ $# -ne 1 ]] && echo "usage: ${0} <target to flash>" && exit 1
[[ ! -e ${1} ]] && echo "Target ${1} does not exist." && exit 2
[[ ! -w ${1} ]] && echo "Target ${1} is not writable. Do you have permissions?" && exit 2

echo "Building image..."
image=$(sudo -u "${SUDO_USER}" ./mmc-image.nix) # cannot always be run as root; TODO: bug report

echo "Flashing ${1}..."
comp_size=$(stat --printf=%s ${image})
comp_ratio=$(zstd -l ${image} | awk 'FNR == 2 {print $7}')
zstdcat ${image} |
    pv -s $(bc <<< "($comp_size * $comp_ratio) / 1") |
    dd of=${1} obs=64k oflag=direct status=none
sync # just in case
