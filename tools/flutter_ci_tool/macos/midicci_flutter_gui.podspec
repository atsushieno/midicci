Pod::Spec.new do |spec|
  spec.name          = 'midicci_flutter_gui'
  spec.version       = '1.0.0'
  spec.license       = { :file => '../LICENSE' }
  spec.homepage      = 'https://github.com/atsushieno/midicci'
  spec.authors       = { 'Atsushi Eno' => 'atsushieno@gmail.com' }
  spec.summary       = 'Flutter GUI for MIDI-CI Tool using midicci C++ library'
  spec.description   = 'A Flutter plugin that provides GUI interface for MIDI-CI operations using the midicci C++ library'

  spec.source        = { :path => '.' }
  spec.source_files  = 'Classes/**/*', '../native/*.{h,cpp}'
  spec.dependency 'FlutterMacOS'

  spec.platform = :osx, '10.14'
  spec.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++20',
    'CLANG_CXX_LIBRARY' => 'libc++',
  }
  spec.swift_version = '5.0'
end
