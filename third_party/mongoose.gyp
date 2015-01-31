{
  "targets": [
    {
      'target_name': 'libmongoose',
      'type': 'static_library',
      'include_dirs': [
        'mongoose/',
      ],
      'sources': [
        'mongoose/mongoose.c',
      ],
    },
  ],
}
