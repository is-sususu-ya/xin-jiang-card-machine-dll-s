#!/bin/bash
# ����һ���������ļ���c���������
echo "" > ./default_cert_config.c
echo "#include <stdio.h>" >> ./default_cert_config.c

echo "const char *default_cert_conf = "  >>  ./default_cert_config.c
cat cacert.pem  | sed '{s/^/&"/}' | sed  '{s/$/\\r\\n"/}' >> default_cert_config.c
echo ";" >> default_cert_config.c
