#!/bin/bash

echo "http_parser download"
echo "from download https://github.com/nodejs/http-parser/archive/refs/tags/v2.9.4.tar.gz"
wget https://github.com/nodejs/http-parser/archive/refs/tags/v2.9.4.tar.gz -O http-parser-2.9.4.tar.gz
tar -zxvf http-parser-2.9.4.tar.gz >/dev/null
rm -f http-parser-2.9.4.tar.gz
mv http-parser-2.9.4 http_parser

echo "pcre2 download"
echo "form download https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.42/pcre2-10.42.tar.gz"
wget https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.42/pcre2-10.42.tar.gz -O pcre2-10.42.tar.gz
tar -zxvf pcre2-10.42.tar.gz >/dev/null
rm -f pcre2-10.42.tar.gz
mv pcre2-10.42 pcre2