function default()
    local b = builder()
    b.compiler = 'gcc'
    b.build_dir = 'build_aite'
    b.bin = 'libmgba.'..builder.dylib_ext
    b.defines = {
        'M_CORE_GB',
        --'M_CORE_GBA',
        'NDEBUG',
        'DEBUGGER_HAX',
		'DISABLE_THREADING',
    }
    if ffi.os == 'Windows' then
        table.insert(b.defines, 'WINDOWS_HAX')
    end
    b.include_dirs = {
        'src',
    }
    b.sflags = '-std=c99'
    b.src = table.merge(
        fs.find('src/core', '*.c'),
        fs.find('src/lr35902', '*.c'),
        fs.find('src/gb', '*.c'),
        fs.find('src/arm', '*.c'),
        --fs.find('src/gba', '*.c'),
        fs.find('src/util', '*.c'),
        ffi.os == 'Windows' and fs.find('src/platform/windows', '*.c') or fs.find('src/platform/posix', '*.c'),
        fs.find('src/third-party/inih', '*.c'),
        fs.find('src/third-party/blip_buf', '*.c'),
        --fs.find('src/debugger', '*.c'),
        'AITE_DECL.c'
    )
    table.removecontents(b.src, table.merge(
		fs.find('src/lr35902/debugger', '*.c'),
		fs.find('src/arm/debugger', '*.c'),
        fs.find('src/core/test', '*.c'),
        fs.find('src/gba/test', '*.c'),
        fs.find('src/gb/test', '*.c'),
        fs.find('src/util/test', '*.c'),
        fs.find('src/util/gui', '*.c'),
        'src/util/vfs/vfs-devlist.c',
        'src/util/vfs/vfs-zip.c',
		'src/util/nointro.c',
		ffi.os == 'Windows' and 'src/util/vfs/vfs-dirent.c'
    ))

    b:link(b:compile())
end
