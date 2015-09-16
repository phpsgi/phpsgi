# PHPSGI

PHP Server Gateway Interface specification

## Abstract

This document specifies a standard interface between web servers and PHP web applications or frameworks. This interface is designed to promote web application portability and reduce the duplication of effort by web application framework developers.

Please keep in mind that PHPSGI is not Yet Another web application framework.  PHPSGI is a specification to decouple web server environments from web application framework code. Nor is PHPSGI a web application API. Web application developers (end users) will not run their web applications directly using the PHPSGI interface, but instead are encouraged to use frameworks that support PHPSGI.

## Terminology

### Web Servers

Web servers accept HTTP requests issued by web clients, dispatching those requests to web applications if configured to do so, and return HTTP responses to the request-initiating clients. Web servers can be any PHP SAPI web server (e.g. php-fpm, Apache PHP module ...) or any HTTP server implemented in PHP like "ReactPHP".

### PHPSGI Server

A PHPSGI Server is a PHP program providing an environment for a PHPSGI application to run in.

PHPSGI specifying an interface for web applications and the main purpose of web applications being to be served to the Internet, a PHPSGI Server will most likely be either: part of a web server (like Apache module PHP), connected to a web server (with FastCGI), invoked by a web server (as in plain old CGI), or be a standalone web server itself, written entirely or partly in PHP.

There is, however, no requirement for a PHPSGI Server to actually be a web server or part of one, as PHPSGI only defines an interface between the server and the application, not between the server and the world.

A PHPSGI Server is often also called PHPSGI Application Container because it is similar to a Java Servlet container, which is Java process providing an environment for Java Servlets.

### Applications

Web applications accept HTTP requests and return HTTP responses.

PHPSGI applications are web applications conforming to the PHPSGI interface, prescribing they take the form of a code reference with defined input and output.

For simplicity, PHPSGI Applications will also be referred to as Applications for the remainder of this document.

### Middleware 

Middleware is a PHPSGI application (a code reference) and a Server. Middleware looks like an application when called from a server, and it in turn can call other applications. It can be thought of a plugin to extend a PHPSGI application.

### Framework developers 

Framework developers are the authors of web application frameworks. They write adapters (or engines) which accept PHPSGI input, run a web application, and return a PHPSGI response to the server.

### Web application developers 

Web application developers are developers who write code on top of a web application framework. These developers should never have to deal with PHPSGI directly.

## Specification

### Application

A PHPSGI application is any callable variable, e.g. a Closure, an array contains callable payload or an object defined with `__invoke` magic method.

To define a PHPSGI application, you just need a closure object with the following prototype):

```php
$app = function(array & $environment, array $response) {
	
	// [response code,  response headers, body content ]
	return [ 200, [ 'Content-Type' => 'plain/text' ], 'Hello World' ];
};
```

The application can also be defined in PHP class:

```php
use PHPSGI\App;

class MyApp implements App {
	public function call(array & $environment, array $response) {
		// [response code,  response headers, body content ]
		return [ 200, [ 'Content-Type' => 'plain/text' ], 'Hello World' ];
	}
	
	public function __invoke(array & $environment, array $response) {
		return $this->call($environment, $response);
	}
}
```

### The Environment

The environment MUST be an associative array reference that includes CGI-like
headers, as detailed below. The application is free to modify the environment.
The environment MUST include these keys (adopted from PEP 333, Rack and JSGI)
except when they would normally be empty.

When an environment key is described as a boolean, its value MUST conform to
PHP's notion of boolean-ness. This means that an empty string or an explicit 0
are both valid false values. If a boolean key is not present, an application
MAY treat this as a false value.

The environment array is derived from `$_SERVER` since it's supported by all current SAPI servers.

To create an environment array from PHP's super global arrays (for SAPI servers):

```php
$environment = array_merge($_SERVER, [
	'parameters' => $_REQUEST,
	'body_parameters' => $_POST,
	'query_parameters' => $_GET,
]);
```

Then the environment array can be passed to the application closure:

```php
$defaultResponse = [ 200, [], '' ];
$response = $app($environment, $defaultResponse);
```


The values for all CGI keys (named without a period) MUST be a scalar string.

See below for details.

* `REQUEST_METHOD`: The HTTP request method, such as "GET" or "POST". This MUST
NOT be an empty string, and so is always required.  `SCRIPT_NAME`: The initial
portion of the request URL's path, corresponding to the application. This tells
the application its virtual "location". This may be an empty string if the
application corresponds to the server's root URI.  If this key is not empty, it
MUST start with a forward slash (`/`).

* `PATH_INFO`: The remainder of the request URL's path, designating the virtual
"location" of the request's target within the application. This may be an empty
string if the request URL targets the application root and does not have a
trailing slash. This value should be URI decoded by servers in order to be
compatible with RFC 3875.  If this key is not empty, it MUST start with a
forward slash (`/`).

* `REQUEST_URI`: The undecoded, raw request URL line. It is the raw URI path and
query part that appears in the HTTP GET /... HTTP/1.x line and doesn't contain
URI scheme and host names.  Unlike `PATH_INFO`, this value SHOULD NOT be
decoded by servers. It is an application's responsibility to properly decode
paths in order to map URLs to application handlers if they choose to use this
key instead of `PATH_INFO`.

### Middleware

package phpsgi/phpsgi provides out-of-box middleware base class and app interface (implemented in both extension and pure php), you can define your middlewares like this:

```php
use PHPSGI\Middleware;

class MyMiddleware extends Middleware
{
	public function call(array & $environment, array $response) {
		// preprocessing ...
		
		// call the next middleware or application
		$response = parent::call($environment, $response);
		
		// postprocessing 
		
		return $response;
	}
}
```

To wrap your application with middlewares:

```php
$app = new MyMiddleware($app);
$app = new XHProfMiddleware($app);
$response = $app($environment, $response);
```


## Questions and Answers

1. Why must environment be an associative array? What's wrong with using a class?

	The rationale for requiring an associative array is to maximize portability between servers. In practice, however, most servers will probably find a Request class adequate to their needs, and thus framework authors will come to expect the full set of features to be available, since they will be there more often than not.
	But, if some server chooses not to use an associative array, then there will be interoperability problems despite that server's "conformance" to spec.
Therefore, making a an associative array mandatory simplifies the specification and guarantees interoperabilty.
2. Why is this interface so low-level? I want feature X! (e.g. cookies, sessions, persistence, ...)

	This isn't Yet Another PHP Web Framework. It's just a way for frameworks to talk to web servers, and vice versa. If you want these features, you need to pick a web framework that provides the features you want. And if that framework lets you create a PHPSGI application, you should be able to run it in most PHPSGI-supporting servers. 
	Also, some PHPSGI servers may offer additional services via objects provided in their environ dictionary; see the applicable server documentation for details. (Of course, applications that use such extensions will not be portable to other PHPSGI-based servers.)
3. Why use CGI variables instead of good old HTTP headers? And why mix them in with PHPSGI-defined variables?

	Many existing web frameworks are built heavily upon the CGI spec, especially the global `$_SERVER` variable, and existing SAPI web servers know how to generate CGI variables. In contrast, alternative ways of representing inbound HTTP information are fragmented and lack market share.
	Thus, using the CGI "standard" seems like a good way to leverage existing implementations. As for mixing them with PHPSGI variables, separating them would just require two assoc array  arguments to be passed around, while providing no real benefits.

## Changelogs


## Acknowledgements

Some parts of this specification are adopted from the following specifications.

- PSGI http://search.cpan.org/~miyagawa/PSGI-1.102/PSGI.pod
- PEP333 Python Web Server Gateway Interface http://www.python.org/dev/peps/pep-0333
- Rack http://rack.rubyforge.org/doc/SPEC.html
- JSGI Specification http://jackjs.org/jsgi-spec.html

I'd like to thank authors of these great documents.

## Author

Yo-An Lin <yoanlin93@gmail.com>

## Copyright and Licenese

Copyright Yo-An Lin, 2015-present

This document is licensed under the Creative Commons license by-sa.




