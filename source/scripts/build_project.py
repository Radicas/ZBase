#!/usr/bin/env python3
"""
ZBase 一键构建、测试、安装脚本

用法示例:
    python source/scripts/build_project.py                    # 默认 Debug + 测试 + 示例
    python source/scripts/build_project.py --asan              # 启用 ASan
    python source/scripts/build_project.py --release           # Release 构建
    python source/scripts/build_project.py --no-tests          # 跳过测试
    python source/scripts/build_project.py --no-examples       # 跳过示例
    python source/scripts/build_project.py --benchmarks        # 包含性能基准
    python source/scripts/build_project.py --install /opt/zbase   # 构建并安装
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


# ---------------------------------------------------------------------------
# 工具函数
# ---------------------------------------------------------------------------

def _rmtree(path: Path) -> None:
    """安全删除目录树（处理 Windows 只读文件）。"""
    if not path.exists():
        return

    def _on_error(func, p, exc_info):
        """尝试去除只读属性后重试。"""
        os.chmod(p, 0o777)
        func(p)

    shutil.rmtree(str(path), onerror=_on_error)


def _read_cache_generator(cache_file: Path) -> str:
    """从 CMakeCache.txt 中读取上次使用的生成器名称。"""
    try:
        for line in cache_file.read_text(encoding="utf-8").splitlines():
            if line.startswith("CMAKE_GENERATOR:INTERNAL="):
                return line.split("=", 1)[1].strip()
    except (OSError, UnicodeDecodeError):
        pass
    return ""

def find_project_root() -> Path:
    """从脚本所在位置向上推导项目根目录（包含 source/ 的目录）。"""
    script_dir = Path(__file__).resolve().parent
    # 脚本在 source/scripts/ 下，项目根为 source/ 的父目录
    if script_dir.parent.name == "source":
        return script_dir.parent.parent
    # 兜底：从当前目录向上找有 source/CMakeLists.txt 的目录
    for parent in Path.cwd().parents:
        if (parent / "source" / "CMakeLists.txt").exists():
            return parent
    sys.exit("错误：无法找到项目根目录（需包含 source/CMakeLists.txt）。请从项目内运行此脚本。")


def find_vsdevcmd() -> str:
    """自动查找 VsDevCmd.bat 路径。"""
    search_roots = [
        os.environ.get("ProgramFiles", "C:/Program Files"),
        os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)"),
    ]

    for root in search_roots:
        vs_dir = Path(root) / "Microsoft Visual Studio"
        if not vs_dir.exists():
            continue
        # 按年份降序排列，优先用最新版
        for year_dir in sorted(vs_dir.iterdir(), reverse=True):
            if not year_dir.is_dir():
                continue
            for edition in ("Community", "Professional", "Enterprise"):
                bat = year_dir / edition / "Common7" / "Tools" / "VsDevCmd.bat"
                if bat.exists():
                    return str(bat)

    sys.exit(
        "错误：未找到 VsDevCmd.bat。请确认已安装 Visual Studio。\n"
        "如果 Visual Studio 安装在非标准路径，请设置环境变量 VSDEVCMD。"
    )


def run_cmd(cmd: str, *, cwd: Path, description: str = "") -> bool:
    """在 cmd.exe 环境中执行命令（已通过 VsDevCmd.bat 初始化 MSVC 环境）。"""
    vsdevcmd = find_vsdevcmd()

    # 将命令写入临时 bat 文件，用 call 链式执行
    bat_content = (
        f'@echo off\r\n'
        f'call "{vsdevcmd}" -arch=x64 > nul\r\n'
        f'cd /d "{cwd}"\r\n'
        f'{cmd}\r\n'
    )

    with tempfile.NamedTemporaryFile(mode="w", suffix=".bat", delete=False, encoding="ascii") as f:
        f.write(bat_content)
        bat_path = f.name

    try:
        print(f"\n--- {description} ---" if description else "")
        result = subprocess.run(
            [bat_path],
            shell=False,
            cwd=str(cwd),
        )
        return result.returncode == 0
    finally:
        try:
            os.unlink(bat_path)
        except OSError:
            pass


# ---------------------------------------------------------------------------
# 构建流程
# ---------------------------------------------------------------------------

def configure(args, project_root: Path, build_dir: Path) -> bool:
    """CMake 配置。"""
    # 检查已有构建目录的生成器是否匹配
    cache_file = build_dir / "CMakeCache.txt"
    if cache_file.exists():
        cache_gen = _read_cache_generator(cache_file)
        if cache_gen and "Visual Studio" not in cache_gen:
            if args.clean:
                print(f"检测到 {cache_gen} 缓存，--clean 已指定，删除重建。")
                _rmtree(build_dir)
                build_dir.mkdir(parents=True, exist_ok=True)
            else:
                print(f"错误：构建目录已有 {cache_gen} 缓存，但当前使用 Visual Studio 生成器。")
                print(f"      请加 --clean 删除后重建，或手动删除 {build_dir}。")
                return False

    build_type = "Release" if args.release else "Debug"

    cmake_opts = f'-DCMAKE_BUILD_TYPE={build_type}'

    if args.asan:
        cmake_opts += ' -DZBASE_ENABLE_ASAN=ON'

    if args.no_tests:
        cmake_opts += ' -DZBASE_BUILD_TESTS=OFF'
    else:
        cmake_opts += ' -DZBASE_BUILD_TESTS=ON'

    if args.no_examples:
        cmake_opts += ' -DZBASE_BUILD_EXAMPLES=OFF'
    else:
        cmake_opts += ' -DZBASE_BUILD_EXAMPLES=ON'

    if args.benchmarks:
        cmake_opts += ' -DZBASE_BUILD_BENCHMARKS=ON'
    else:
        cmake_opts += ' -DZBASE_BUILD_BENCHMARKS=OFF'

    if args.analyze:
        cmake_opts += ' -DZBASE_ENABLE_ANALYZE=ON'

    if args.static:
        cmake_opts += ' -DBUILD_SHARED_LIBS=OFF'

    build_dir.mkdir(parents=True, exist_ok=True)

    cmd = (
        f'cmake -S "{project_root / "source"}" '
        f'-B "{build_dir}" '
        f'-G "Visual Studio 18 2026" -A x64 '
        f'{cmake_opts}'
    )
    return run_cmd(cmd, cwd=project_root, description=f"CMake 配置 ({build_type} 模式)")


def build(args, build_dir: Path) -> bool:
    """CMake 构建。"""
    config = "Release" if args.release else "Debug"
    cmd = f'cmake --build "{build_dir}" --config {config}'
    return run_cmd(cmd, cwd=build_dir, description="构建中...")


def run_tests(args, build_dir: Path) -> bool:
    """运行单元测试。"""
    config = "Release" if args.release else "Debug"
    cmd = f'ctest --output-on-failure -C {config}'
    return run_cmd(cmd, cwd=build_dir, description="运行单元测试")


def _print_installed_tree(prefix: Path) -> None:
    """打印安装目录的树形结构。"""
    if not prefix.exists():
        return
    print(f"\n安装目录结构 ({prefix}):")
    for root, dirs, files in os.walk(str(prefix)):
        level = root.replace(str(prefix), "").count(os.sep)
        indent = "  " * level
        print(f"{indent}{os.path.basename(root)}/")
        sub_indent = "  " * (level + 1)
        for f in files:
            filepath = Path(root) / f
            size = filepath.stat().st_size
            print(f"{sub_indent}{f}  ({_format_size(size)})")


def _format_size(size: int) -> str:
    """格式化文件大小（人类可读）。"""
    for unit in ("B", "KB", "MB", "GB"):
        if abs(size) < 1024:
            return f"{size:.1f} {unit}"
        size /= 1024
    return f"{size:.1f} TB"


def install(args, build_dir: Path) -> bool:
    """执行 CMake install。"""
    config = "Release" if args.release else "Debug"
    prefix = Path(os.path.abspath(args.install)) if args.install else None

    if prefix:
        prefix_opt = f'-DCMAKE_INSTALL_PREFIX="{prefix}"'
        # 需要重新配置以设置安装前缀
        configure_cmd = (
            f'cmake -S "{args.project_root / "source"}" '
            f'-B "{build_dir}" '
            f'-G "Visual Studio 18 2026" -A x64 '
            f'{prefix_opt}'
        )
        if not run_cmd(configure_cmd, cwd=args.project_root, description="设置安装前缀"):
            return False

    # --verbose 显示每个文件的安装详情
    cmd = f'cmake --install "{build_dir}" --config {config} --verbose'
    if not run_cmd(cmd, cwd=build_dir, description=f"安装到 {prefix or '默认路径'}"):
        return False

    # 安装示例程序（CMake install 不包含 examples，手动拷贝）
    if not args.no_examples and prefix:
        _install_examples(build_dir, config, prefix)

    # 打印安装汇总
    if prefix and prefix.exists():
        _print_installed_tree(prefix)
    print(f"\n安装完成，目标路径: {prefix or '（CMake 默认安装路径）'}")

    return True


def _install_examples(build_dir: Path, config: str, prefix: Path) -> None:
    """将构建产物中的示例程序拷贝到安装目录的 bin/ 下。"""
    config_dir = build_dir / config
    if not config_dir.exists():
        return

    # 排除列表：编译器探测程序、测试程序、主库 DLL
    exclude_names = {
        "CompilerIdC.exe",
        "CompilerIdCXX.exe",
        "zbase_tests.exe",
        "zbase.dll",
    }

    copied = []
    for exe in sorted(config_dir.glob("*.exe")):
        if exe.name in exclude_names:
            continue
        dest_dir = prefix / "bin"
        dest_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(str(exe), str(dest_dir / exe.name))
        copied.append(exe.name)

    if copied:
        print(f"\n已安装 {len(copied)} 个示例程序到 {prefix / 'bin'}:")
        for name in copied:
            size = (prefix / "bin" / name).stat().st_size
            print(f"  {name}  ({_format_size(size)})")


# ---------------------------------------------------------------------------
# 入口
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="ZBase 一键构建、测试、安装脚本",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
    python source/scripts/build_project.py
    python source/scripts/build_project.py --release
    python source/scripts/build_project.py --asan
    python source/scripts/build_project.py --asan --release
    python source/scripts/build_project.py --install ./output
    python source/scripts/build_project.py --benchmarks
        """,
    )

    parser.add_argument("--release", action="store_true", help="Release 构建（默认 Debug）")
    parser.add_argument("--asan", action="store_true", help="启用 AddressSanitizer")
    parser.add_argument("--analyze", action="store_true", help="启用 MSVC 静态分析 /analyze")
    parser.add_argument("--benchmarks", action="store_true", help="包含性能基准测试")
    parser.add_argument("--static", action="store_true", help="编译为静态库（默认动态库）")
    parser.add_argument("--no-tests", action="store_true", help="跳过单元测试")
    parser.add_argument("--no-examples", action="store_true", help="跳过示例程序")
    parser.add_argument("--install", metavar="PATH", help="安装到指定路径（如 ./output）")
    parser.add_argument("--skip-tests", action="store_true", help="构建后跳过测试运行")
    parser.add_argument("--clean", action="store_true", help="构建前清理已有构建目录")

    args = parser.parse_args()

    project_root = find_project_root()
    args.project_root = project_root

    # 构建目录命名：build[_asan][_release]
    suffix = ""
    if args.asan:
        suffix += "_asan"
    if args.analyze:
        suffix += "_analyze"
    if args.release:
        suffix += "_release"
    if args.static:
        suffix += "_static"
    build_dir = project_root / f"build{suffix}"

    print(f"项目根目录:  {project_root}")
    print(f"构建目录:    {build_dir}")
    print(f"构建类型:    {'Release' if args.release else 'Debug'}")
    print(f"ASan:        {'启用' if args.asan else '关闭'}")
    print(f"静态分析:    {'启用' if args.analyze else '关闭'}")
    print(f"单元测试:    {'关闭' if args.no_tests else '启用'}")
    print(f"示例程序:    {'关闭' if args.no_examples else '启用'}")
    print(f"性能基准:    {'启用' if args.benchmarks else '关闭'}")
    print(f"库类型:      {'静态' if args.static else '动态'}")

    # 步骤 1: 配置
    if not configure(args, project_root, build_dir):
        sys.exit("CMake 配置失败。")

    # 步骤 2: 构建
    if not build(args, build_dir):
        sys.exit("构建失败。")

    # 步骤 3: 测试（可选）
    if not args.no_tests and not args.skip_tests:
        if not run_tests(args, build_dir):
            sys.exit("单元测试未通过。")

    # 步骤 4: 安装（可选）
    if args.install:
        if not install(args, build_dir):
            sys.exit("安装失败。")

    print("\n=== 全部完成 ===")


if __name__ == "__main__":
    main()
