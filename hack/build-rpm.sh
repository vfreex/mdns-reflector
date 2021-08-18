#!/bin/bash
set -euo pipefail
SELF=$(readlink -f -- "$0")
HERE=$(dirname -- "$SELF")

cd "$HERE"/..

NAME=$(rpmspec -q --qf "%{name}" --srpm ./mdns-reflector.spec)
VERSION=$(rpmspec -q --qf "%{version}" --srpm ./mdns-reflector.spec)
RELEASE_WITH_DIST=$(rpmspec -q --qf "%{release}" --srpm ./mdns-reflector.spec)
RELEASE=$(rpmspec -q --qf "%{release}" --undefine dist --srpm ./mdns-reflector.spec)

rm -rf build/rpmbuild
mkdir -p build/rpmbuild/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
tar -czf build/rpmbuild/SOURCES/"$NAME-$VERSION".tar.gz \
  --exclude=.git --exclude=build --exclude='cmake-*' \
  --transform="s,^\./,$NAME-$VERSION/," \
  .
cp ./mdns-reflector.spec build/rpmbuild/SPECS
cd "$HERE"/../build/rpmbuild
pwd
rpmbuild --define "_topdir $PWD" -v -ba SPECS/mdns-reflector.spec
# mock --buildsrpm --spec ./mdns-reflector.spec --source build/rpmbuild/SOURCES/$NAME-$VERSION.tar.gz --resultdir=./build/mock  # -r epel-8-$(arch)
# mock --rebuild ./build/mock/$NAME-$VERSION-$RELEASE_WITH_DIST.src.rpm --resultdir=./build/mock  # -r epel-8-$(arch)
