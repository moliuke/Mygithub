<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2010. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<TITLE>协议配置</TITLE>
<link rel="stylesheet" type="text/css" href="w3c.css">


<script>
	function onChangeFristProtocol()
	{
		var option = document.getElementById("selectFristProtocal").value;
		console.log(option);
		document.getElementsByName("protocol")[0].value = option;
	}
</script>
<script>
	function onChangeSecondProtocol()
	{
		var option = document.getElementById("selectSecondProtocal").value;
		console.log(option);
		document.getElementsByName("swr_protocol")[0].value = option;
	}
</script>
</head>
<div id="header">
<img src="swr.jpg" align ="left" width="160" height = "70">
<h2>工控机配置</h2>
</div>
<div id="nav">
<nobr><A HREF="ip.asp">IP配置</A>
<A HREF="protocol.asp">协议选择</A>
<A HREF="config.asp">功能选择</A>
<A HREF="upload.asp">工控机升级</A></nobr>
</div>
<body>
<table>

<div id="section">
<div id="myborder">

<p style='color:#0000ff'>一级协议:显科、金晓、成都、厦门、治超、中海治超、紫光、modbus、维护模式</p>
<p style='color:#0000ff'>二级协议:通用、亳州、福州、荷坳、河北二秦、马拉西亚、珠海</p>
<p style='color:#000000'>一级协议选择:</p>
<select onchange="onChangeFristProtocol()" id="selectFristProtocal">
  <option value="seewor">显科</option>
  <option value ="jinxiao">金晓</option>
  <option value ="chengdu">成都</option>
  <option value="xiamen">厦门</option>
  <option value="zhichao">治超</option>
  <option value ="zhonghaizc">中海治超</option>
  <option value ="perplelight">紫光</option>
  <option value="modbus">modbus</option>
  <option value="upgrade">维护模式</option>
</select>
<p style='color:#000000'>二级级协议选择:</p>
<select onchange="onChangeSecondProtocol()" id="selectSecondProtocal">
  <option value ="general">通用</option>
  <option value ="bozhou">亳州</option>
  <option value="fuzhou">福州</option>
  <option value="heao">荷坳</option>
  <option value ="hebeierqin">河北二秦</option>
  <option value ="malaysia">马拉西亚</option>
  <option value="zhuhaiproj">珠海</option>
</select>
<form action="/goform/formProtocol" method="POST">
            <br>
            一级协议:<input readonly=“readonly” type="text" name="protocol" value="<%GetProtocol('protocol'); %>" size="16" maxlength="64">
            <br>
            <br>
            二级协议:<input readonly=“readonly” type="text" name="swr_protocol" size="16" value="<%GetProtocol('swr_protocol'); %>"  maxlength="64">
            <br>
            <br>
			<button type="submit" value="发送">发送</button>
			<button type="reset" value="重置">重置</button>
        </form>            

<div/>
<div/>
</table>
<div id="footer">
<hr/>
Page refresh at <!--#t--> @ seewor.com
</div>

</body>
</html>