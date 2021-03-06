import os
import subprocess

# LIBWEBSOCKETS_REV = "e6bd6296bd9f90f6a264ea9e69825552e8702490" # v2.3.0
# LIBWEBSOCKETS_REV = "a085a0ab3838401c7ff9bf2ad8646af633746a14" # v1.7.9
# LIBWEBSOCKETS_REV = "2209ceb3f5ff3cfad7af9a81327012d15d0d5094" # v1.7.1
LIBWEBSOCKETS_REV = "34266b786162656f48e63aa81c39f55dc6c72015" # v1.7.2

def _download_libwebsockets(SCRIPT_DIR, DOWNLOAD_DIR):
    print(" -> downloading libwebsockets")
    os.chdir(DOWNLOAD_DIR)
    subprocess.run(
        "git clone https://github.com/warmcat/libwebsockets.git libwebsockets",
        shell=True,
        check=True
    )
    os.chdir(os.path.realpath(os.path.join(DOWNLOAD_DIR, "libwebsockets")))
    subprocess.run("git checkout %s" % LIBWEBSOCKETS_REV, shell=True, check=True)
    os.chdir(SCRIPT_DIR)

def _build_libwebsockets(SCRIPT_DIR, DOWNLOAD_DIR):
    print(" -> building libwebsockets")

    INSTALL_DIR = os.path.realpath(os.path.join(DOWNLOAD_DIR, "libwebsockets_install"))
    OPENSSL_PATH = os.path.realpath(os.path.join(DOWNLOAD_DIR, "openssl"))

    os.chdir(os.path.realpath(os.path.join(DOWNLOAD_DIR, "libwebsockets")))

    build_dir = os.path.realpath(os.path.join(os.path.join(DOWNLOAD_DIR, "libwebsockets"), "build"))
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    os.chdir(build_dir)

    subprocess.run("cmake .. -DOPENSSL_ROOT_DIR=%s -DCMAKE_INSTALL_PREFIX=%s" % (OPENSSL_PATH, INSTALL_DIR), shell=True, check=True)
    # subprocess.run("MSBuild libwebsockets.sln /p:Configuration=Release", shell=True, check=True)
    subprocess.run("cmake --build . --config Release --target install", shell=True, check=True)

    os.chdir(SCRIPT_DIR)

def build_libwebsockets(SCRIPT_DIR, DOWNLOAD_DIR):
    _download_libwebsockets(SCRIPT_DIR, DOWNLOAD_DIR)
    _build_libwebsockets(SCRIPT_DIR, DOWNLOAD_DIR)
