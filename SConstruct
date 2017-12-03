from glob import iglob

Program(
    target = 'server',
    source = list(iglob('src/*.c')) + list(iglob('src/**/*.c')),
    CCFLAGS = '-g -Werror'
)

