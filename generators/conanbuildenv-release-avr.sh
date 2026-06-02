script_folder="/home/david/dev/smart-bell/generators"
echo "echo Restoring environment" > "$script_folder/deactivate_conanbuildenv-release-avr.sh"
for v in STRIP CXX CC RANLIB AS AR PATH CONAN_CMAKE_FIND_ROOT CHOST
do
   is_defined="true"
   value=$(printenv $v) || is_defined="" || true
   if [ -n "$value" ] || [ -n "$is_defined" ]
   then
       echo export "$v='$value'" >> "$script_folder/deactivate_conanbuildenv-release-avr.sh"
   else
       echo unset $v >> "$script_folder/deactivate_conanbuildenv-release-avr.sh"
   fi
done

export STRIP="/opt/avr8-gnu-toolchain-linux_x86_64/bin/avr-strip"
export CXX="/opt/avr8-gnu-toolchain-linux_x86_64/bin/avr-g++"
export CC="/opt/avr8-gnu-toolchain-linux_x86_64/bin/avr-gcc"
export RANLIB="/opt/avr8-gnu-toolchain-linux_x86_64/bin/avr-ranlib"
export AS="/opt/avr8-gnu-toolchain-linux_x86_64/bin/avr-as"
export AR="/opt/avr8-gnu-toolchain-linux_x86_64/bin/avr-ar"
export PATH="/opt/avr8-gnu-toolchain-linux_x86_64/bin:$PATH"
export CONAN_CMAKE_FIND_ROOT="/opt/avr8-gnu-toolchain-linux_x86_64"
export CHOST="avr"