{
  "targets": [
    {
      'target_name': 'libmongoose',
      'type': 'static_library',
      'include_dirs': [
        'mongoose/',
      ],
      'defines': [
        'MONGOOSE_NO_AUTH',
        'MONGOOSE_NO_CGI',
        'MONGOOSE_NO_DAV',
        'MONGOOSE_NO_LOGGING',
        'MONGOOSE_NO_WEBSOCKET',
      ],
      'sources': [
        'mongoose/mongoose.c',
      ],
    },
  ],
}
