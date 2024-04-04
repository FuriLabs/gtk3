#!/bin/sh
# Copyright 2024 Simon McVittie
# SPDX-License-Identifier: LGPL-2.1-or-later

# Reproducer for <https://bugs.debian.org/1065494>.
# Needs to be run as root in an expendable amd64 chroot, container or VM
# that initially has apt sources that can install some version of
# libgtk-3-0, for example:
# podman run --rm -it -v $(pwd):$(pwd):ro -w $(pwd) debian:bookworm-slim debian/tests/manual/1065494.sh
# podman run --rm -it -v $(pwd):$(pwd):ro -w $(pwd) debian:sid-slim debian/tests/manual/1065494.sh

set -eux

export DEBIAN_FRONTEND=noninteractive
n=0
failed=0
this_tuple=x86_64-linux-gnu

assert () {
    n=$(( n + 1 ))

    if "$@"; then
        echo "ok $n - $*"
    else
        echo "not ok $n - $* exit status $?"
        failed=1
    fi
}

# GTK 3 contains some IM modules, so we don't need to install a separate
# package to get a non-empty cache.
apt-get -y update
apt-get -y install libgtk-3-0
test -e "/usr/lib/$this_tuple/gtk-3.0/3.0.0/immodules.cache"
test -s "/usr/lib/$this_tuple/gtk-3.0/3.0.0/immodules.cache"

# Upgrade to current unstable, with libgtk-3-0t64
cat > /etc/apt/sources.list.d/debian.sources <<EOF
Types: deb
URIs: http://deb.debian.org/debian
Suites: sid
Components: main
Signed-By: /usr/share/keyrings/debian-archive-keyring.gpg
EOF

# Add a deb822-formatted apt source at this location if you are testing a
# locally-built gtk+3.0 before upload
if [ -e "debian/tests/manual/local-1065494.sources" ]; then
    install -m644 "debian/tests/manual/local-1065494.sources" /etc/apt/sources.list.d/
fi

# Reproducer: Upgrade to libgtk-3-0t64. This runs the postrm from
# libgtk-3-0, which deletes necessary files.
apt-get -y update
apt-get -y install --purge libgtk-3-0t64

assert test -e "/usr/lib/$this_tuple/gtk-3.0/3.0.0/immodules.cache"
assert test -s "/usr/lib/$this_tuple/gtk-3.0/3.0.0/immodules.cache"

# Workaround: Trigger the postinst of libgtk-3-0t64, which will regenerate
# the generated files.
apt-get -y install --reinstall libgtk-3-0t64

assert test -e "/usr/lib/$this_tuple/gtk-3.0/3.0.0/immodules.cache"
assert test -s "/usr/lib/$this_tuple/gtk-3.0/3.0.0/immodules.cache"

echo "1..$n"
exit "$failed"

# vim:set sw=4 sts=4 et:
