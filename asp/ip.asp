<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2010. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<TITLE>IP配置</TITLE>
<link rel="stylesheet" type="text/css" href="w3c.css">

<script>
	function networkcheck()
	{
		obj_ip = document.getElementById("ip").value;
		var exp=/^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$/;
		
		obj_netmask = document.getElementById("netmask").value;
		obj_gateway = document.getElementById("gateway").value;
		var port = document.getElementById("port").value;
		var reg = /^(254|252|248|240|224|192|128|0)\.0\.0\.0|255\.(254|252|248|240|224|192|128|0)\.0\.0|255\.255\.(254|252|248|240|224|192|128|0)\.0|255\.255\.255\.(254|252|248|240|224|192|128|0)$/;
		if(obj_ip.match(exp) != null && obj_gateway.match(exp) != null && obj_netmask.match(reg) != null && port>1023 && port <49152)
		{
			var val = document.getElementById("IpSumbit");
			val.click();
		}	
		else if(obj_ip.match(exp) == null || obj_gateway.match(exp) == null)
		{
			alert("ip或网关格式输入不合法！");
			return false;
			
		}
		else if(port < 1024 || port > 49151)
		{
			alert("端口号的输入范围请在1024~49151之间！");
			return false;
		}
		else if(obj_netmask.match(reg) == null)
		{
			alert("子网掩码输入不合法！");
			return false;
		}
		else
		{
			alert("未知错误！");
			return false;
		}
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
<div id="section">
<div id="myborder">

<table>
<tr>
<td>
<form action="/goform/formIP" method="POST">
            <br>
            <p>IP 地 址</p><input type="text" id="ip" name="ip" value="<%GetConfig('ip'); %>" size="16" maxlength="15">
            <br>
            <br>
            <p>子网掩码</p><input type="text" id="netmask" name="netmask" size="16" value="<%GetConfig('netmask'); %>"  maxlength="15">
            <br>
            <br>
            <p>网    关</p><input type="text" id="gateway" name="gateway" value="<%GetConfig('gateway'); %>" size="16" maxlength="15">
            <br>
            <br>
            <p>端 口 号</p><input type="text" id="port" name="port" value="<%GetConfig('port'); %>" size="16" maxlength="5">
            <br>
            <br>
			
			<input type="button" onclick="networkcheck()" value="提交" style="background-color: teal;color: white;border-radius: 4px;padding: 2px 15px 2px 15px;"></input>
			<input type="submit" id="IpSumbit" hidden="hidden"></input>
			<button type="reset" value="重置">重置</button>

        </form>            
</td>
</tr>

</table>

</div>
</div>
<div id="footer">
<hr/>
Page refresh at <!--#t--> @ seewor.com
</div>
</body>
</html>

