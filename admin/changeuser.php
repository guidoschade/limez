<html>
<head><title>Change</title>
<link rel=stylesheet type="text/css" href="format.css">
</head>
<body text='#000000' bgcolor='#AAAABE'>
<?php

  include ("limez.ini");
  include ("limez.lib");

  if (con($limez_host, $limez_port, $timeout, &$fp, $limez_pass, "LCMD $action $list $user") == 1)
  {
    echo "<b>Server Response: </b><br>$x<br>";
    while(1)
    {
      $x = fgets ($fp,128);
      if (strncmp($x, $end, 4))
        break;
      echo substr($x,4) . "<br>";
    }
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
