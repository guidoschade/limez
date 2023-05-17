<html>
  <head>
    <title>Limez Documentation</title>
    <link rel=stylesheet type="text/css" href="limezformate.css">
  </head>

<BODY text='#000000' bgcolor='#ffffff'>

<a name="top"></a>
<center><H2>LiMez Configuration Syntax</H2></center>

<p>
<table align="center" width="70%" border="0">
<tr>
      <td width="50%" id="tbdkl">
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite"><b>
           main settings</b></font></div>
      </td>
      <td width="50%" id="tbdkl">
        <div align="center"> <font face="Arial, Helvetica, sans-serif" id="lite"><b>
           list settings</b></font></div>
      </td>
    </tr>

<?php

  include ("doc.ini");

  function cmp ($a, $b)
  {
     return strcmp($a[0],$b[0]);
  }

  usort($descr, "cmp");
  reset($descr);

  $count = count($descr);
  for ($i = 0; $i < $count; $i++)
  {
    if ($descr[$i][5] & 1)
      $server[] = $descr[$i];
    if ($descr[$i][5] & 2)
      $list[] = $descr[$i];
  }

  $scount = count($server);
  $lcount = count($list);

  for ($i = 0; $i < $scount; $i++)
  {
    echo "<tr>";
    echo "<td id=\"tbhel\"><tt><a href=\"#".$server[$i][0]."\"><font id=\"dark\">\n";
    echo "".$server[$i][0]."&nbsp;</font></a></tt></td>\n";
    echo "<td id=\"tbhel\"><tt><a href=\"#".$list[$i][0]."\"><font id=\"dark\">\n";
    echo "".$list[$i][0]."&nbsp;</font></a></tt></td>\n";
    echo "</tr>\n";
  }
  echo "</table>";

  for ($i = 0; $i < $count; $i++)
  {
    echo "\n<br>\n<table align=center width=90% cellspacing=0>\n<tr>\n";
    echo "<td width=\"11%\" id=\"tbdkl\"><font id=\"var\">Entry</font></td>\n";
    echo "<td width=\"55%\" id=\"tbhel\"><font id=\"vft\"><a name=\"".$descr[$i][0]."\">\n";
    echo "<a href=\"#top\">".$descr[$i][0]."</td></a></font>\n";
    echo "<td width=\"9%\"  id=\"tbdkl\"><font id=\"var\">Default</td></font>\n";
    echo "<td width=\"25%\" id=\"tbhel\"><font id=\"val\">".$descr[$i][1]."</td></font>\n";
    echo "</tr>\n";
    echo "<tr>\n";
    echo "<td width=\"11%\" id=\"tbdkl\"><font id=\"var\">Description</td></font>\n";
    echo "<td width=\"55%\" id=\"tbhel\"><font id=\"val\">".$descr[$i][2]."</td></font>\n";
    echo "<td width=\"9%\"  id=\"tbdkl\"><font id=\"var\">Version</td></font>\n";
    echo "<td width=\"25%\" id=\"tbhel\"><font id=\"val\">".$descr[$i][3]."</td></font>\n";
    echo "</tr>\n";
    echo "<tr>\n";
    echo "<td width=\"11%\" id=\"tbdkl\"><font id=\"var\">Example</td></font>\n";
    echo "<td width=\"89%\" id=\"tbhel\" colspan=\"3\"><font id=\"val\">\n";
    echo "".$descr[$i][0]." \"".$descr[$i][4]."\"</td></font>\n";
    echo "</tr>\n";
    echo "</table>\n";
  }
?>


</body>


</html>
