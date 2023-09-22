#!/bin/bash
# 这是一个读配置文件到c代码的例子
echo "" > ./default_cert_config.c
echo "#include <stdio.h>" >> ./default_cert_config.c

echo "const char *default_cert_conf = "  >>  ./default_cert_config.c
cat cacert.pem  | sed '{s/^/&"/}' | sed  '{s/$/\\r\\n"/}' >> default_cert_config.c
echo ";" >> default_cert_config.c
