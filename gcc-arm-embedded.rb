cask 'gcc-arm-embedded' do
  version '4_8-2014q3'
  sha256 '6b30901738b09a8d22fdfff99e991217444b80ac492a6163af5c06a3baaa3487'

  # launchpad.net/gcc-arm-embedded/ was verified as official when first introduced to the cask
  url "https://launchpad.net/gcc-arm-embedded/4.8/4.8-2014-q3-update/+download/gcc-arm-none-eabi-#{version}-20140805-mac.tar.bz2"
  name 'GCC ARM Embedded'
  homepage 'https://developer.arm.com/open-source/gnu-toolchain/gnu-rm'

  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-addr2line"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-ar"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-as"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-c++"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-c++filt"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-cpp"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-elfedit"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-g++"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-gcc"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-gcc-ar"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-gcc-nm"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-gcc-ranlib"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-gcov"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-gdb"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-gprof"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-ld"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-ld.bfd"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-nm"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-objcopy"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-objdump"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-ranlib"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-readelf"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-size"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-strings"
  binary "gcc-arm-none-eabi-#{version}/bin/arm-none-eabi-strip"
end
