#!/usr/bin/php-cgi
<?php
header("Content-Type: text/plain");
echo "Method: " . $_SERVER['REQUEST_METHOD'] . "\n";

if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    $body = file_get_contents("php://input");
    echo "POST Data: " . $body . "\n";
} elseif ($_SERVER['REQUEST_METHOD'] == 'DELETE') {
    echo "DELETE request handled.\n";
} else {
    echo "Use GET, POST, or DELETE methods.\n";
}
?>