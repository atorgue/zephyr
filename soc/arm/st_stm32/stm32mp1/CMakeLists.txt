zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )
zephyr_sources_ifdef(CONFIG_SOC_STM32MP15_RPROC_RSC_TABLE resource_table.c)