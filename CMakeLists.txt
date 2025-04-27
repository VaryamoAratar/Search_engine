# Минимальная версия CMake
cmake_minimum_required(VERSION 3.16)

# Имя проекта
project(SearchEngine)

# Установка стандартов C++ и обязательность
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Указываем путь к vcpkg toolchain и триплет
set(CMAKE_TOOLCHAIN_FILE "D:/Code/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")

# Находим зависимости
find_package(Boost REQUIRED COMPONENTS system filesystem regex locale asio beast)
find_package(libpqxx CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

# Добавляем папку с заголовками
include_directories(include)

# Находим все исходники в каталоге src
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Определяем минимальную версию Windows
add_compile_definitions(_WIN32_WINNT=0x0601)

# Создаем исполнимый файл
add_executable(SearchEngine ${SOURCES})

# Линковка с необходимыми библиотеками
target_link_libraries(SearchEngine
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::locale
    Boost::beast
    Boost::asio
    OpenSSL::SSL
    OpenSSL::Crypto
    libpqxx::pqxx
    PostgreSQL::PostgreSQL
)

# Указываем, что проект использует C++20
set_target_properties(SearchEngine PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)
