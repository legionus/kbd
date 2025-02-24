#!/bin/bash -efu

PROG="${0##*/}"
message() {
	printf >&2 '%s: %s\n' "$PROG" "$*"
}

run() {
	message "run: $*"; "$@"
}

create_file() {
	message "creating $1 ..."
	cat > "$1"
}

workdir="/tmp/${CI_ARCH-}"

mkdir -p -- "$workdir"

image="$workdir/cloud.img"
ssh_type="ed25519"
ssh_key="$workdir/qemu-ssh-key"

[ -f "$ssh_key" ] ||
	run ssh-keygen -t "$ssh_type" -C "ci@kbd-project.org" -N "" -f "$ssh_key"

uuid=
read -r uuid < /proc/sys/kernel/random/uuid ||:

pubkey=
read -r pubkey < "$ssh_key.pub" ||:

# More information: https://cloudinit.readthedocs.io/en/latest/reference/index.html

create_file /tmp/user-data.yaml <<EOF
#cloud-config
resize_rootfs: true
ssh_pwauth: true
disable_root: false
chpasswd:
  expire: false
  users:
  - {name: ubuntu, password: mypassword, type: text}
users:
  - name: ubuntu
    groups: users, admin, wheel, tty, sudo
    sudo: ALL=(ALL) NOPASSWD:ALL
    shell: /bin/bash
    ssh_authorized_keys:
      - $pubkey
mounts:
- [ hostfs, /opt, 9p, "ro", "0", "0" ]
EOF

create_file /tmp/meta-data.yaml <<EOF
instance-id: $uuid
local-hostname: cxl01
EOF

create_file /tmp/network-config.yaml <<EOF
version: 2
ethernets:
  eth0:
    match:
      name: e*
    dhcp4: true
    dhcp6: true
    nameservers:
      addresses:
        - 8.8.8.8
        - 8.8.4.4
        - 2001:4860:4860::8888
        - 2001:4860:4860::8844
EOF

run cloud-localds -v \
	--network-config=/tmp/network-config.yaml "$image" \
	/tmp/user-data.yaml /tmp/meta-data.yaml

