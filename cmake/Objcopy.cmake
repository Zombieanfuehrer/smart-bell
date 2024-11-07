set(OBJCOPY /opt/avr8-gnu-toolchain-linux_x86_64/bin/avr-objcopy)

function(run_bin2hex)
    cmake_parse_arguments(
        RUN_BIN2HEX
        ""
        "TARGET;TARGET_ARTIFACT;HEX_FILE"
        ""
        ${ARGN})

    add_custom_command(
        OUTPUT ${RUN_BIN2HEX_HEX_FILE}
        COMMAND ${OBJCOPY} -O ihex "${RUN_BIN2HEX_TARGET_ARTIFACT}" ${RUN_BIN2HEX_HEX_FILE}
        DEPENDS ${RUN_BIN2HEX_TARGET_ARTIFACT}
        COMMENT "Converting ${RUN_BIN2HEX_TARGET_ARTIFACT} to hex file")

    add_custom_target(${RUN_BIN2HEX_TARGET}_hex ALL DEPENDS ${RUN_BIN2HEX_HEX_FILE})
endfunction()
