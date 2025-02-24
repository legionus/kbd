#!/bin/bash -efu

timeout=0
sleep_seconds() {
	sleep $1
	timeout=$(( $timeout - $1 ))
}

set -x

version="${CI_UBUNTU_VERSION:-24.10}"
arch="${CI_ARCH-}"

if [ -z "$arch" ]; then
	arch="$1"
	shift
fi

volume="$1"
shift

workdir="/tmp/$arch"
mkdir -p -- "$workdir"

qemu_args=()
case "$arch" in
	x86_64)
		qemu_arch="x86_64"
		url="https://cloud-images.ubuntu.com/releases/$version/release/ubuntu-$version-server-cloudimg-amd64.img"
		qemu_args+=(
			-enable-kvm
			-cpu host -machine "accel=kvm:tcg"
		)
		;;
	s390x)
		qemu_arch="s390x"
		url="https://cloud-images.ubuntu.com/releases/$version/release/ubuntu-$version-server-cloudimg-s390x.img"
		qemu_args+=(
			-cpu max -accel tcg
		)
		;;
	ppc64el)
		qemu_arch="ppc64"
		url="https://cloud-images.ubuntu.com/releases/$version/release/ubuntu-$version-server-cloudimg-ppc64el.img"
		qemu_args+=(
			-cpu POWER10 -accel tcg
			-prom-env "input-device=/vdevice/vty@71000000"
			-prom-env "output-device=/device/vty@71000000"
		)
		;;
	*)
		: "unknown architecture: $1"
		exit 1
		;;
esac

qcow2="$workdir/server.img"
cloud="$workdir/cloud.img"
pidfile="$workdir/qemu.pid"
logfile="$workdir/qemu.log"

if [ ! -s "$qcow2" ]; then
	while ! curl -L --retry-delay 1 --retry 7 -o "$qcow2" "$url"; do
		: "unable to download image: $url"
	done
fi

: "resizing image ..."

qcow2_size="$(qemu-img info "$qcow2" | sed -nr -e 's|^virtual size: .* \(([0-9]+) bytes\)|\1|p')"
qemu-img resize "$qcow2" $(( $qcow2_size * 2 ))

: "starting Ubuntu-$version VM ..."

nproc=$(grep -c ^processor /proc/cpuinfo || echo 2)

"qemu-system-$qemu_arch" "${qemu_args[@]}" \
	-pidfile "$pidfile" \
	-smp $nproc -m 8G -nographic -no-reboot \
	-net nic -net user,hostfwd=tcp::2222-:22 \
	-drive file="$qcow2",if=virtio \
	-drive file="$cloud",if=virtio \
	-virtfs "local,id=virtfs-0,path=$volume,security_model=none,mount_tag=hostfs,multidevs=remap" \
	>"$logfile" 2>&1 &

timeout=$(( 60 * 1 ))

while [ "$timeout" -gt 0 ] && [ ! -s "$pidfile" ]; do
	sleep_seconds 1
done

[ -s "$pidfile" ] || {
	: "failed to start qemu"
	exit 1
}

pid=
read -r pid < "$pidfile" ||:

timeout=$(( 60 * 10 ))

while [ "$timeout" -gt 0 ]; do
	sleep_seconds 3

	if netstat -nlt | grep -qsE '^tcp .* 0\.0\.0\.0:2222 .* LISTEN'; then
		timeout=
		break
	fi

	: "waiting for Ubuntu-$version VM (pid=$pid) to be ready ..."
done

if [ -n "$timeout" ]; then
	: "Ubuntu-$version VM (pid=$pid) did not appear to be ready. killing ..."
	kill "$pid"
	wait
	exit 1
fi

: "Ubuntu-$version VM (pid=$pid) is ready"
