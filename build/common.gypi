{
  'variables': {
  },

  'target_defaults': {
    'includes': [
      'filename_rules.gypi',
    ],
    'include_dirs': [
      '<(DEPTH)',
    ],
    'default_configuration': 'Debug',
    'configurations': {
      'Release': {
        'defines': [
          'NDEBUG',
        ],
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '-O3',
              '-fdata-sections',
              '-ffunction-sections',
            ],
            'ldflags': [
              '-Wl,-O1',
              '-Wl,--gc-sections',
            ],
          }],  # OS=='linux'

          ['OS=="win"', {
            'variables': {
              'optimize%': 'size',
            },
            'target_conditions': [
              ['optimize=="size"', {
                  'msvs_settings': {
                    'VCCLCompilerTool': {
                      # 1, optimizeMinSpace, Minimize Size (/O1)
                      'Optimization': '1',
                      # 2, favorSize - Favor small code (/Os)
                      'FavorSizeOrSpeed': '2',
                    },
                  },
                },
              ],
              ['optimize=="speed"', {
                  'msvs_settings': {
                    'VCCLCompilerTool': {
                      # 2, optimizeMaxSpeed, Maximize Speed (/O2)
                      'Optimization': '2',
                      # 1, favorSpeed - Favor fast code (/Ot)
                      'FavorSizeOrSpeed': '1',
                    },
                  },
                },
              ],
              ['optimize=="max"', {
                  'msvs_settings': {
                    'VCCLCompilerTool': {
                      # 2, optimizeMaxSpeed, Maximize Speed (/O2)
                      'Optimization': '2',
                      # 1, favorSpeed - Favor fast code (/Ot)
                      'FavorSizeOrSpeed': '1',
                      # This implies link time code generation.
                      'WholeProgramOptimization': 'true',
                    },
                  },
                },
              ],
            ],
          }],  # OS==win
        ],
      },
      'Debug': {
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '-O0',
              '-g'
            ],
          }],
        ],
      },
    },
  },  # target_defaults

  'conditions': [
    ['OS=="linux"', {
      'target_defaults': {
        'cflags': [
          '-fPIC',
          '-fno-exceptions',
          '-fno-strict-aliasing',
          '-fstack-protector-all',  # Implies -fstack-protector
          '-fvisibility=hidden',
          '-g',
          '-pipe',
          '-pthread',
          '-Wall',
          '-Wextra',
          '-Wno-unused-parameter',
          '-Wno-missing-field-initializers',
        ],
        'cflags_cc': [
          '-fno-rtti',
          '-fno-threadsafe-statics',
          '-fvisibility-inlines-hidden',
          '-Wsign-compare',
          '-mfpmath=sse',
        ],
        'defines': [
          '_FILE_OFFSET_BITS=64',
        ],
        'ldflags': [
          '-fPIC',
          '-pthread',
          '-Wl,--as-needed',
          '-Wl,-z,noexecstack',
        ],
      }
    }], # OS==linux

    ['OS=="win"', {
      'target_defaults': {
        'defines': [
          '_WIN32_WINNT=0x0603',
          'WINVER=0x0603',
          'WIN32',
          '_WINDOWS',
          'NOMINMAX',
          'PSAPI_VERSION=1',
          '_CRT_RAND_S',
          'CERT_CHAIN_PARA_HAS_EXTRA_FIELDS',
          'WIN32_LEAN_AND_MEAN',
          '_ATL_NO_OPENGL',
          '_SECURE_ATL',
          # _HAS_EXCEPTIONS must match ExceptionHandling in msvs_settings.
          '_HAS_EXCEPTIONS=0',
        ],
        'msvs_system_include_dirs': [
        ],
        'msvs_disabled_warnings': [
          '4702',
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'AdditionalOptions': ['/MP'],
            'MinimalRebuild': 'false',
            'BufferSecurityCheck': 'true',
            'EnableFunctionLevelLinking': 'true',
            'RuntimeTypeInfo': 'false',
            'WarningLevel': '4',
            'WarnAsError': 'false',
            'DebugInformationFormat': '3',
            # ExceptionHandling must match _HAS_EXCEPTIONS above.
            'ExceptionHandling': '0',
          },
          'VCLibrarianTool': {
            'AdditionalOptions': ['/ignore:4221'],
            'AdditionalLibraryDirectories': [
            ],
          },
          'VCLinkerTool': {
            'AdditionalDependencies': [
              'Advapi32.lib',
              'wininet.lib',
              'dnsapi.lib',
              'version.lib',
              'msimg32.lib',
              'ws2_32.lib',
              'usp10.lib',
              'psapi.lib',
              'dbghelp.lib',
              'winmm.lib',
              'shlwapi.lib',
            ],
            'AdditionalLibraryDirectories': [
            ],
            'GenerateDebugInformation': 'true',
            'MapFileName': '$(OutDir)\\$(TargetName).map',
            'ImportLibrary': '$(OutDir)\\lib\\$(TargetName).lib',
            'FixedBaseAddress': '1',
            # SubSystem values:
            #   0 == not set
            #   1 == /SUBSYSTEM:CONSOLE
            #   2 == /SUBSYSTEM:WINDOWS
            # Most of the executables we'll ever create are tests
            # and utilities with console output.
            'SubSystem': '1',
          },
          'VCMIDLTool': {
            'GenerateStublessProxies': 'true',
            'TypeLibraryName': '$(InputName).tlb',
            'OutputDirectory': '$(IntDir)',
            'HeaderFileName': '$(InputName).h',
            'DLLDataFileName': '$(InputName).dlldata.c',
            'InterfaceIdentifierFileName': '$(InputName)_i.c',
            'ProxyFileName': '$(InputName)_p.c',
          },
          'VCResourceCompilerTool': {
            'Culture' : '1033',
            'AdditionalIncludeDirectories': [
              '<(DEPTH)',
              '<(SHARED_INTERMEDIATE_DIR)',
            ],
          },
          'target_conditions': [
            ['_type=="executable"', {
              'VCManifestTool': {
                'EmbedManifest': 'true',
              },
            }],
          ],
        },
      },
    }],
  ],
}
