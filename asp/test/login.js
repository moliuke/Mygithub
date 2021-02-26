function getLength(str){
    return str.replace(/[^\x00-xff]/g,"xx").length;//使用替换函数将正则值替换为xx
    }
    function findStr(str,n){//检测是否密码为连续的相同字符
        var tmp=0;
        for(var i=0;i<str.length;i++){
            if(str.charAt(i)==n){
                tmp++;
              }
        }
        return tmp;
    }
    
var code = ""; //验证码
//生成验证码
function createCode(){
    code = "";//重新初始化验证码
    var num = 4; //验证码位数
    var codeList = new Array(1,2,3,4,5,6,7,8,9,0,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'); //验证码内容
    //循环获取每一位验证码
        for(var i = 0; i < num; i++){
            //随机数 * 验证码候选元素数量 = 候选元素数组下标
            code += codeList[Math.floor(Math.random() * codeList.length)];
        }
        document.getElementById("txtCode").value = code;
}
     
 window.onload=function (){
            createCode(); //加载验证码
           var alnput=document.getElementsByTagName('input');
           var oName=alnput[0];
           var pwd=alnput[1];
           var pwd2=alnput[2];
           var ma1=alnput[3];
           var ma2=alnput[4];
           var aP=document.getElementsByTagName('p');//ap中的P为大写
           var name_msg=aP[0];
           var pwd_msg=aP[1];
           var pwd2_msg=aP[2];
           var ma=aP[3];
           var count=document.getElementById('count');
           var aEm=document.getElementsByTagName('em');
           var name_length=0;
           //1.数字、字母(不分大小写)、汉字、下划线
           //2. 5-25字符，推荐使用中文
           //\u4e00-\u9fa5(这是汉字的范围)
           var re=/[^\w\u4e00-\u9fa5]/g;
           var re_n=/[^\d]/g;
           var re_t=/[^a-zA-Z]/g;
           
           //用户名
           oName.onfocus=function(){
               name_msg.style.display="block";
               name_msg.innerHTML='<i class="ati"></i>5-25个字符';
           }//获取焦点事件
           oName.onkeyup=function(){
               count.style.visibility="visible";//有疑惑
               name_length=getLength(this.value);
               count.innerHTML=name_length+"个字符";
               if(name_length==0){
                   count.style.visibility="hidden"; 
               }
           }//键盘事件
           oName.onblur=function(){
               //含有非法字符
                 var re=/[^\w\u4e00-\u9fa5]/g;
                 if(re.test(this.value)){
                     name_msg.innerHTML='<i class="err"></i>含有非法字符';
                 }
               //不能为空
               else if(this.value==""){
                     name_msg.innerHTML='<i class="err"></i>不能为空';
               }
               //长度超过25个字符
               else if(name_length>25){
                     name_msg.innerHTML='<i class="err"></i>长度超过25个字符';
               }
               //长度少于6个字符
               else if(name_length<6){
                     name_msg.innerHTML='<i class="err"></i>长度少于6个字符';
               }
               //ok
               else{
                   name_msg.innerHTML='<i class="ok"></i>ok';
                   }
           }//失去焦点事件
           //密码
           pwd.onfocus=function(){
               pwd_msg.style.display="block";
               pwd_msg.innerHTML='<i class="ati"></i>6-16个字符，请使用字母加数字或符号的组合密码，不能单独使用字母、数字或符号。';
           }
           pwd.onkeyup=function(){
               //大于5字符"中"
               if(this.value.length>5){
                   aEm[1].className="active";
                   pwd2.removeAttribute("disabled");//移除禁用属性
                   pwd2_msg.style.display="block";
               }
               else{
                  aEm[1].className="";
                  pwd2_msg.style.display="none";
                  pwd2.disabled="true";//重新设置为禁用状态
                 
               }
               //大于10个字符"强"
                if(this.value.length>10){
                   aEm[2].className="active";
                   
                   pwd2_msg.style.display="block";
               }
               else{
                  aEm[2].className="";
               }
               
           }
           pwd.onblur=function(){
               var m=findStr(pwd.value,pwd.value[0]);
               //不能为空
               if(this.value==""){
                   pwd_msg.innerHTML='<i class="err"></i>不能为空';
               }
               //不能用相同字符
               else if(m==this.value.length){
                   pwd_msg.innerHTML='<i class="err"></i>不能使用相同字符';
               }
               //长度应为6-16个字符
               else if(this.value.length<6||this.value.length>16){
                   pwd_msg.innerHTML='<i class="err"></i>长度应为6-16个字符';
               }
               //不能全为数字
               else if(!re_n.test(this.value)){
                    pwd_msg.innerHTML='<i class="err"></i>不能全为数字';
               }
               //不能全为字母
                 else if(!re_t.test(this.value)){
                    pwd_msg.innerHTML='<i class="err"></i>不能全为字母';
               }
                //ok
               else{
                   pwd_msg.innerHTML='<i class="err"></i>ok';
                   
               }
              
           }
           //确认密码
          
           pwd2.onblur=function(){
               if(this.value!=pwd.value){
                   pwd2_msg.innerHTML='<i class="err"></i>两次输入密码不一致！';   
               }
               else{
                    pwd2_msg.innerHTML='<i class="err"></i>ok！'; 
               }
           }
           //验证码
            ma1.onblur=function(){
                if(ma1.value==code){    
            ma.style.display="block";
               ma.innerHTML='<i class="ati"></i>ok';
                }
    else{
        ma.style.display="block";
        ma.innerHTML='<i class="ati"></i>验证码输入有误';
        code = "";
    createCode(); //生成新的验证码
        }
            }
                 
 }
 //下面为check函数进行提交前的验证
   function check(){
       
       var alnput=document.getElementsByTagName('input');
           var oName=alnput[0];
           var pwd=alnput[1];
           var pwd2=alnput[2];
           var ma1=alnput[3];
           var ma2=alnput[4];
           var subt=alnput[5];
           var aP=document.getElementsByTagName('p');//ap中的P为大写
           var name_msg=aP[0];
           var pwd_msg=aP[1];
           var pwd2_msg=aP[2];
           var count=document.getElementById('count');
           var aEm=document.getElementsByTagName('em');
           var name_length=0;
           //1.数字、字母(不分大小写)、汉字、下划线
           //2. 5-25字符，推荐使用中文
           //\u4e00-\u9fa5(这是汉字的范围)
           var re=/[^\w\u4e00-\u9fa5]/g;
           var re_n=/[^\d]/g;
           var re_t=/[^a-zA-Z]/g;
           var k=1;
           var re=/[^\w\u4e00-\u9fa5]/g;
           //用户名
           name_length=getLength(oName.value);
            if (oName.value==""){
               
               name_msg.style.display="block";
               name_msg.innerHTML='<i class="ati"></i>请输入用户名';
               k=k+1;
            }
            else if(re.test(oName.value)){
                 name_msg.innerHTML='<i class="err"></i>含有非法字符';
                 k=k+1;
            }
            //长度超过25个字符
               else if(name_length>25){
                     //name_msg.innerHTML='<i class="err"></i>长度超过25个字符';
                     k=k+1;
               }
               //长度少于6个字符
               else if(name_length<6){
                    // name_msg.innerHTML='<i class="err"></i>长度少于6个字符';
                      k=k+1;
               }
            else{
                k=k+0;
            }
           
            //密码
            var m=findStr(pwd.value,pwd.value[0]);
               //不能为空
               if(pwd.value==""){
               pwd_msg.style.display="block";
              // pwd_msg.innerHTML='<i class="ati"></i>不能为空';
               k=k+1;
               }
               //不能用相同字符
               else if(m==pwd.value.length){
                   k=k+1;
               }
               //长度应为6-16个字符
               else if(pwd.value.length<6||pwd.value.length>16){
                   k=k+1;
               }
               //不能全为数字
               else if(!re_n.test(pwd.value)){
                    k=k+1;
               }
               //不能全为字母
                 else if(!re_t.test(pwd.value)){
                    k=k+1;
               }
                //ok
               else{
                   k=k+0;
                   
               }
             //确认密码
              if(pwd2.value!=pwd.value){
                   pwd2_msg.innerHTML='<i class="err"></i>两次输入密码不一致！';  
                   k=k+1; 
               } 
               else{
                   k=k+0;
               }
             //检验验证码输入是否有误
             if(ma1.value!=ma2.value){
                 k=k+1;
             }
             else{
                 k=k+0;
             }
            //下面的操作计算check函数的返回值
            if(k!=1){
                return false;
            }
            else{
                  aEm[1].className="";
                  subt.value='正在提交';
                  subt.disabled='true';
                return true; 
            }    
       
 }
      function register(){
   	window.open("http://127.0.0.1:8020/16210320312-%E5%8C%85%E4%B8%B9%E9%9C%9E/register.html")
   } 
   
// window.onload = function(){
// 	var btn = document.getElementById('btn');
// 	btn.onclick = function(){
// 		window.open('register.html');
// 	}
// }
