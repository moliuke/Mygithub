<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2010. All Rights Reserved. ->
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>Log</title>
</head>

<body>
<table>
<tr>
<td>
<H3> 配置：IP地址 子网掩码 网关 端口号 </H3>
<form action="/goform/formRegister" method="POST">
            <br>
            IP地址:<input type="text" name="ip" value="<%GetConfig('ip'); %>" size="16" maxlength="64">
            <br>
            <br>
            子网掩码:<input type="text" name="netmask" size="16" value="<%GetConfig('netmask'); %>"  maxlength="64">
            <br>
            <br>
            网关:<input type="text" name="gateway" value="<%GetConfig('gateway'); %>" size="16" maxlength="64">
            <br>
            <br>
            端口号:<input type="text" name="port" value="<%GetConfig('port'); %>" size="16" maxlength="64">
            <br>
            <br>
            <input type="submit" value="提交">
            <input type="reset" value="重置">
        </form>            
</td>
</tr>

</table>



</body>
</html>
