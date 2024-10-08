# - Select IP Stack To Build Project For
#
# This function selects one of the supported IP stacks to be built into the
# project (see documentation for supported IP stacks)
#
# ip_stack - uppercase name of one of the supported IP stacks
# ip_includes - list of required include paths to support building the stack
#               these would be specific to the integrators project
function(setup_bm_ip_stack ip_stack ip_includes)
    set(BM_IP_STACK
        ${ip_stack}
        CACHE
        INTERNAL
        "ip stack"
        FORCE
    )
    set(BM_IP_INCLUDES
        "${ip_includes}"
        CACHE
        INTERNAL
        "ip includes"
        FORCE
    )
endfunction()

# - Select Operating System To Build Project For
#
# This function selects one of the supported Operating Systems to be built into
# the project (see documentation for supported OS)
#
# os - uppercase name of one of the supported OS
# ip_includes - list of required include paths to support building a supported
#               OS, these would be specific to the integrators project
function(setup_bm_os os os_includes)
    set(BM_OS
        ${os}
        CACHE
        INTERNAL
        "os"
        FORCE
    )
    set(BM_OS_INCLUDES
        "${os_includes}"
        CACHE
        INTERNAL
        "os includes"
        FORCE
    )
endfunction()
