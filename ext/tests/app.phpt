--TEST--
Check PHPSGI\App
--FILE--
<?php
use PHPSGI\App;
class FooApp implements App {
    public function call(array & $environment, array $response)
    {
        return [ ];
    }
}
--EXPECT--
