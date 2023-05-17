<html>
<head><title>Config</title>
<link rel=stylesheet type="text/css" href="format.css">
</head>
<body text='#000000' bgcolor='#AAAABE'>
<?php

  include ("limez.ini");
  include ("limez.lib");
  $change = 0;

  if ($action == "change")
  {
    $change = 1;
    $action = "ldel";
  }

  if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "$action config $list $key $value") == 1)
  {
    $x = fgets ($fp,128);

    // response in status frame
    if ($x[0]=="2")
    {
      echo "<b>Server Response: </b><br>$x<br>";

      if ($change)
      {
        fputs ($fp, "LADD config $list $key $newval\r\n");
        $x = fgets ($fp,128);
        echo "$x<br>";
      }

      // reread config
      sleep(1);
      fputs ($fp, "LRLD\r\n");
      $x = fgets ($fp,128);
      echo "$x<br>";
    }
    else
      echo "<b>Server Response: </b><br><font color=red>$x</font><br>";

    endcon ($fp);
  }

?>

<script language="JavaScript">
<!--
parent.right.location.reload();
//-->
</script>
</body>
</html>
