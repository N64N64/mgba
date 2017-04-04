function default()
    local b = builder()
    b.compiler = 'gcc'
    b.build_dir = 'build_aite'
    b.bin = 'libmgba.'..builder.dylib_ext
    b.defines = {
        'M_CORE_GB',
        'M_CORE_GBA',
        'NDEBUG',
    }
    b.include_dirs = {
        'src',
    }
    b.sflags = '-std=c99'
    b.src = table.merge(
        fs.find('src/core', '*.c'),
        fs.find('src/lr35902', '*.c'),
        fs.find('src/gb', '*.c'),
        fs.find('src/arm', '*.c'),
        fs.find('src/gba', '*.c'),
        fs.find('src/util', '*.c'),
        fs.find('src/platform/posix', '*.c'),
        fs.find('src/third-party/inih', '*.c'),
        fs.find('src/third-party/blip_buf', '*.c'),
        fs.find('src/debugger', '*.c'),
        'AITE_DECL.c'
    )
    table.removecontents(b.src, table.merge(
        fs.find('src/core/test', '*.c'),
        fs.find('src/gba/test', '*.c'),
        fs.find('src/gb/test', '*.c'),
        fs.find('src/util/test', '*.c'),
        fs.find('src/util/gui', '*.c'),
        'src/util/vfs/vfs-devlist.c',
        'src/util/vfs/vfs-zip.c'
    ))

    b:link(b:compile())
end
