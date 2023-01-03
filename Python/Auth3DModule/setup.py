from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            name="auth3d",
            
            include_dirs=["../../DivaLib/src"],
            library_dirs=["../../x64/Release"],
            
            sources=["src/module.cpp"],
            libraries=["divalib"],
            language='c++',
            extra_compile_args=['/std:c++17']
        ),
    ]
)