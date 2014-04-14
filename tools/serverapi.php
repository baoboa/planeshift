<?php

$ip = "127.0.0.1";
$code = "abcd";

// Change your database settings here:
$db = mysql_connect("localhost", "planeshift", "planeshift") or die ("Unable to connect!"); // host, user, pass
mysql_select_db("planeshift", $db) or die("Could not select database");

if($_SERVER["REMOTE_ADDR"] != $ip || $_GET["code"] != $code)
{
	echo "Access DENIED!";
	exit();
}

if($_GET["function"] == "add_user")
{
	mysql_query("INSERT into accounts (username,password,last_login_ip,security_level,status) VALUES\n(\"".$_GET["user"]."\",\"".$_GET["pass"]."\",\"127.0.0.1\", 0, \"A\")") or die("fatal error: couldn't insert stuff: " . mysql_error() . "\n");

}
else if($_GET["function"] = "edit_pass")
{
		mysql_query("UPDATE accounts set password=\"".$_GET["pass"]."\" where username=\"".$_GET["user"]."\"") or die("fatal error: couldn't insert stuff: " . mysql_error() . "\n");
}
else
{
	echo "unimplemented function!";
}

?>
