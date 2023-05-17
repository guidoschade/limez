<html>
<head><title>LiMez Main Config</title>
<link rel=stylesheet type="text/css" href="format.css">
</head>
<body text='#000000' bgcolor='#aaaabe'>

<?php

  include ("doc.ini");
  $found = 0;

  $count = count($descr);
  for ($i = 0; $i < $count; $i++)
  {
    if (!strcmp($key,$descr[$i][0]))
    {
      $found = 1;
      break;
    }
  }

  if ($found == 1)
  {
    echo "<table bgcolor=#ffffff align=center width=90% cellspacing=0><tr>\n";
    echo "<td width=\"11%\" id=\"tbdkl\"><font id=\"var\">Entry</font></td>\n";
    echo "<td width=\"55%\"><font id=\"vft\">$key</font></td>\n";
    echo "<td width=\"9%\"  id=\"tbdkl\"><font id=\"var\">Default</font></td>\n";
    echo "<td width=\"25%\"><font id=\"val\">".$descr[$i][1]."</font></td>\n";
    echo "</tr>\n";
    echo "<tr >\n";
    echo "<td width=\"11%\" id=\"tbdkl\"><font id=\"var\">Description</font></td>\n";
    echo "<td width=\"55%\"><font id=\"val\">".$descr[$i][2]."</font></td>\n";
    echo "<td width=\"9%\"  id=\"tbdkl\"><font id=\"var\">Version</font></td>\n";
    echo "<td width=\"25%\"><font id=\"val\">".$descr[$i][3]."</font></td>\n";
    echo "</tr>\n";
    echo "<tr >\n";
    echo "<td width=\"11%\" id=\"tbdkl\"><font id=\"var\">Example</font></td>\n";
    echo "<td width=\"89%\" colspan=\"3\"><font id=\"val\">".$key." \"".$descr[$i][4]."\"</font></td>\n";
    echo "</tr>\n";
    echo "</table>\n";
  }
  else
    echo "<br><br><H3>key '$key' unknown, please update limez-admin</H3>\n";
?>


</body>


</html>
