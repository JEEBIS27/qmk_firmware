#pragma once
typedef enum { OS_UNSURE, OS_LINUX, OS_MACOS, OS_IOS } os_variant_t;
os_variant_t detected_host_os(void);
