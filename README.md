
This is a very simple Apache 2 module to control exaclty what client-side headers will
be passed over to your server-side applications to reduce possible attack surface and
support in-depth security.

### Compiling (RHEL)

```
$ sudo dnf install httpd httpd-devel apr apr-util redhat-rpm-config
$ make
```


### Installing

The makefile provided adds module to your httpd.conf automatically:

```
sudo make install
```

It will create httpd.conf record and activate it:

```
LoadModule header_whitelist_module /usr/lib64/httpd/modules/mod_header_whitelist.so
```

To activate module, restart apache httpd:

```
sudo systemctl restart httpd
```

### Configuration 

This module supports the following parameters.

#### HeadersClientWhitelist

Space-separated list of client headers that are whitelisted (case-insensitive).

Can be set in global or in virtual host context.

Example:
```
HeadersClientWhitelist Host User-Agent Accept Cookie Set-Cookie Authorization
```

#### HeadersClientSensitive

It is possible for debugging purposes to log out every HTTP header, it's value and decision
to allow/strip it out. However, some headers might contain sensitive information that should
be better not logged. For this purpose, one can define list of sensitive headers whose values
will never be saved into log files.

This parameter defines a space-separated list of client headers whose values should not be 
logged into server's logfiles.

The parameter can be set in global or in virtual host context.

Example: 
```
HeadersClientSensitive Cookie Set-Cookie Authorization
```

### Testing

Virtual host definition I used for testing (assuming 192.168.56.100 is the host-only network 
for your VMs).

```httpd.conf
HeadersClientWhitelist Host User-Agent Accept Cookie Set-Cookie Authorization

HeadersClientSensitive Cookie Set-Cookie Authorization

# IP-based virtual host for tests (no ServerName needed)
<VirtualHost 192.168.56.100:80>
    DocumentRoot "/var/www/html/iptest"
    ErrorLog "/var/log/httpd/iptest_error.log"
    CustomLog "/var/log/httpd/iptest_access.log" combined
    # Increase verbosity to see debug logging in error log
    LogLevel header_whitelist_module:debug
</VirtualHost>
```

The /var/www/html/iptest directory might contain simple index.html file:

```
$ cat /var/www/html/iptest/index.html
It works!
```

From Linux CLI:

```
$ curl -v http://192.168.56.100/ -H "X-Test: bad" -H "User-Agent: myagent" -H "Authorization: secret"

$ sudo tail -f /var/log/httpd/iptest_error.log
```

Logs:
```
[:debug] ... [client XXXX ] whitelist: allowed header: Host: 192.168.56.100
[:debug] ... [client XXXX ] whitelist: allowed header: Accept: */*
[:debug] ... [client XXXX ] whitelist: stripped header: X-Test: bad
[:debug] ... [client XXXX ] whitelist: allowed header: User-Agent: myagent
[:debug] ... [client XXXX ] whitelist: allowed header: Authorization: <hidden>
```

To make sure headers are really stripped off, I use php to show actual headers:


```
$ sudo dnf install php
$ suso systemctl restart httpd
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
URL -> http://192.168.56.100/index.php

```text
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:143.0) Gecko/20100101 Firefox/143.0
Host: 192.168.56.100
Authorization: secret
```

### Debugging

To log each request headers, as well add decision about it, use "LogLevel header_whitelist_module:debug" 
parameter. Use "HeadersClientSensitive" parameter to prevent sensitive header values to be logged.
