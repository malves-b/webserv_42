#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");
echo "<h2>PHP CGI Echo</h2>";

if (!empty($_GET)) {
    echo "<p><b>GET Parameters:</b></p><ul>";
    foreach ($_GET as $k => $v) echo "<li>$k = $v</li>";
    echo "</ul>";
}

if (!empty($_POST)) {
    echo "<p><b>POST Parameters:</b></p><ul>";
    foreach ($_POST as $k => $v) echo "<li>$k = $v</li>";
    echo "</ul>";
}

echo "<hr><p><a href='/'>Back to Home</a></p>";
?>