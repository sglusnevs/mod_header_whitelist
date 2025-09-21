
This is a very simple Apache 2 module to control exaclty what client-side headers will
be passed over to your server-side applications.

### Compiling (RHEL)

```
dnf install httpd httpd-devel apr apr-util redhat-rpm-config
make
```


### Installing

Makefile provided automatically adds module to your httpd.conf, but you might need
to change the HeaderWhitelist configuration parameter to match your needs.

```
sudo make install
systemctl restart httpd
```

Virtual host definition (192.168.56.100 is the host-only network for my VMs)

```
# Allow only these headers to pass through
HeaderWhitelist Host User-Agent Accept

# IP-based virtual host for tests (no ServerName needed)
<VirtualHost 192.168.56.100:80>
    DocumentRoot "/var/www/html/iptest"
    ErrorLog "/var/log/httpd/iptest_error.log"
    CustomLog "/var/log/httpd/iptest_access.log" combined
    # Increase verbosity to see debug logging in error log
    LogLevel header_whitelist_module:debug
</VirtualHost>
```

### Testing

From Linux CLI:

```
curl -v http://192.168.56.122/ -H "X-Test: bad" -H "User-Agent: myagent"

tail -f /var/log/httpd/iptest_error.log
```

To make sure headers are really stripped off, I used php to show actual headers:


```
dnf install php
systemctl restart httpd
```

```
$ cat /var/www/html/iptest/index.php
<pre><?php

$headers =  getallheaders();
foreach($headers as $key=>$val){
  echo $key . ': ' . $val . '<br>';
}
```


In browser:
URL -> http://192.168.56.100/

```text
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:143.0) Gecko/20100101 Firefox/143.0
Host: 192.168.56.100
```
