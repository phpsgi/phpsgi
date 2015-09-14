<?php
namespace PHPSGI;

interface App
{
    /**
     * @param array $environment
     * @param array $response
     *
     * @return array
     */
    public function call(array & $environment, array $response);

}


