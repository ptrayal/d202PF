<?php

$dbserver = "localhost";
$dbname = "gicker_d20mud";
$dbpasswd = "Flyboy56";
$dbuser = "gicker_d20mud";

mysql_connect($dbserver,$dbuser,$dbpasswd);

mysql_select_db($dbname);

$query = "SELECT a.id_msg AS msgone, a.player_name AS pname, b.id_msg AS msgtwo FROM player_note_read a LEFT JOIN player_note_messages b ON a.id_msg=b.id_msg ORDER BY a.id_msg ASC";

$res = mysql_query($query) or die(mysql_error());

while ($row = mysql_fetch_array($res)) {
  if (!$row['msgone']) {
    echo "DELETING MSG ID: ".$row['msgone']." FOR ".$row['pname']."\n";
    $query = "DELETE FROM player_note_read WHERE id_msg='".$row['msgone']."'";
    mysql_query($query) or die(mysql_error());
  }
}

mysql_close();

?>
