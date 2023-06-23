add_rules("mode.debug", "mode.release") 
set_rundir("$(projectdir)")

if is_mode ("debug") then 
    add_defines("DEBUG") 
else 
    add_defines("NDEBUG") 
end
target("cat") 
    set_kind("binary") 
    add_files("src/mycat.c")

target("cp") 
    set_kind("binary") 
    add_files("src/mycp.c")

target("sys") 
    set_kind("binary") 
    add_files("src/mysys.c")

target("sh") 
    set_kind("binary") 
    add_files("src/sh.c")

target("sh2") 
    set_kind("binary") 
    add_files("src/sh2.c")

target("cat2")
    set_kind("binary")
    add_files("src/mycat2.c")

target("echo")
    set_kind("binary")
    add_files("src/myecho.c")

target("read_pipe") 
    set_kind("binary") 
    add_files("src/read_pipe.c")

target("pi1") 
    set_kind("binary")
    add_files("src/pi1.c")

target("pi2")
    set_kind("binary") 
    add_files("src/pi2.c")

target("sort")
    set_kind("binary") 
    add_files("src/sort.c")

target("pc1") 
    set_kind("binary")
    add_files("src/pc1.c")

target("pc2") 
    set_kind("binary") 
    add_files("src/pc2.c")

target("sfind") 
    set_kind("binary") 
    add_files("src/sfind.c")

target("pfind") 
    set_kind("binary") 
    add_files("src/pfind.c")

target("httpd_serial") 
    set_rundir("$(projectdir)/src/job10/http/serial")
    set_kind("binary") 
    add_files("src/job10/http/serial/http.c") 
    add_files("src/job10/http/serial/main.c")
    add_files("src/job10/http/serial/tcp.c")

target("httpd_concurrent") 
    set_rundir("$(projectdir)/src/job10/http/concurrent")
    set_kind("binary") 
    add_files("src/job10/http/concurrent/http.c") 
    add_files("src/job10/http/concurrent/main.c")
    add_files("src/job10/http/concurrent/tcp.c")

target("pfc")
    set_rundir("$(projectdir)/src/coro")
    set_kind("binary")
    add_files("src/coro/pfc.c")
    add_files("src/coro/coro.c")
    add_files("src/coro/cpu.S")

target("co_find")
    set_rundir("$(projectdir)/src/coro")
    set_kind("binary")
    add_files("src/coro/find.c")
    add_files("src/coro/coro.c")
    add_files("src/coro/cpu.S")
    add_files("src/coro/fs_walk.c")

target("nju_os_sh")
    set_rundir("$(projectdir)/src/nju_os")
    set_kind("binary")
    add_files("src/nju_os/sh.c")