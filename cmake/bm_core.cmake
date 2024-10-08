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
