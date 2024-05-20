#!/bin/sh

# organize folders
cd ..
rm -rf release
rm -rf build
mkdir -p release/example
cp -rf src/pcsx2_ipc.h release/
cp -rf windows-qt.pro meson.build src/ release/example/
cp -rf bindings/ release/

# generate docs
doxygen
mkdir -p release/docs
cp -rf html release/docs
cd latex && make
cd ..
cp -rf latex/refman.pdf release/docs

# remove build artifacts
find release -type d -name build -prune -exec rm -rf {} \;
find release -type d -name bin -prune -exec rm -rf {} \;
find release -type d -name obj -prune -exec rm -rf {} \;
find release -type d -name libpcsx2_ipc_c.so -prune -exec rm -rf {} \;
find release -type d -name target -prune -exec rm -rf {} \;

# we restart our virtual X server for test cases, our test cases kill it.
killall Xvfb
Xvfb :99 &
# test cases, to see if we've broken something between releases
rm -rf build
meson build -Db_coverage=true
cd build
if meson test; then
    echo "Tests ran successfully, time to build the release!"
else
    RED='\033[0;31m'
    NC='\033[0m' # No Color
    echo -e "${RED}TESTS FAILED!!!\nYou broke it, PEBKAC${NC}"
    # make sure i don't forget to read the logs
    rm -rf ../release
    rm -rf ../release.zip
    exit 1
fi

# we build the coverage report and finish the release folder
ninja coverage-html
ninja coverage-xml
mkdir -p ../release/tests
cp -rf meson-logs/coveragereport/ ../release/tests
python ../utils/pretty-tests.py meson-logs/testlog.json > ../release/tests/result.txt
cp -rf ../LICENSE ../release/
cp -rf ../RELEASE_README.md ../release/README.md
cd ..

# make the release zip
zip -r release.zip release &> /dev/null
