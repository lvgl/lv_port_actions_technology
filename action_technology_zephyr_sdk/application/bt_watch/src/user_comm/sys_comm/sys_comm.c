/*!
 * \file	  
 * \brief	  
 * \details   
 * \author	  
 * \date	  
 * \copyright Actions
 */

#include <user_comm/sys_comm.h>


/*!
 * \brief Loop buffer write data
 * \n  Return the actual write data length
 */
u32_t loop_buffer_write(loop_buffer_t* loop_buf, void* data, u32_t size)
{
	// u32_t  irq_flags = local_irq_save();
	
	if (size > loop_buf->buf_size - loop_buf->data_count)
	{
		size = loop_buf->buf_size - loop_buf->data_count;
	}

	if (size == 0)
	{
		goto end;
	}
	
	if (loop_buf->write_pos + size <= loop_buf->buf_size)
	{
		memcpy(&loop_buf->data_buf[loop_buf->write_pos], data, size);

		loop_buf->write_pos += size;
	}
	else
	{
		u32_t  len = loop_buf->buf_size - loop_buf->write_pos;

		if (len > 0)
		{
			memcpy(&loop_buf->data_buf[loop_buf->write_pos], data, len);
		}
		
		memcpy(&loop_buf->data_buf[0], (u8_t*)data + len, size - len);

		loop_buf->write_pos = size - len;
	}

	loop_buf->data_count += size;

end:
	// local_irq_restore(irq_flags);

	return size;
}


/*!
 * \brief Loop buffer read data
 * \n  Return the actual read data length
 */
u32_t loop_buffer_read(loop_buffer_t* loop_buf, void* data, u32_t size)
{
	// u32_t  irq_flags = local_irq_save();
	
	if (size > loop_buf->data_count)
	{
		size = loop_buf->data_count;
	}

	if (size == 0)
	{
		goto end;
	}

	if (loop_buf->read_pos + size <= loop_buf->buf_size)
	{
		if (data != NULL)
		{
			memcpy(data, &loop_buf->data_buf[loop_buf->read_pos], size);
		}

		loop_buf->read_pos += size;
	}
	else
	{
		u32_t  len = loop_buf->buf_size - loop_buf->read_pos;

		if (data != NULL)
		{
			if (len > 0)
			{
				memcpy(data, &loop_buf->data_buf[loop_buf->read_pos], len);
			}
			
			memcpy((u8_t*)data + len, &loop_buf->data_buf[0], size - len);
		}

		loop_buf->read_pos = size - len;
	}

	loop_buf->data_count -= size;
	
end:
	// local_irq_restore(irq_flags);

	return size;
}

/* 实现itoa函数的源码 */ 
char *myitoa(int num,char *str,int radix)
{  
	/* 索引表 */ 
	const char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; 
	unsigned unum; /* 中间变量 */ 
	int i=0,j,k; 
	/* 确定unum的值 */ 
	if(radix==10&&num<0) /* 十进制负数 */ 
	{ 
		unum=(unsigned)-num; 
		str[i++]='-'; 
	} 
	else unum=(unsigned)num; /* 其它情况 */ 
	/* 逆序 */ 
	do	
	{ 
		str[i++]=index[unum%(unsigned)radix]; 
		unum/=radix; 
	}while(unum); 
	str[i]='\0'; 
	/* 转换 */ 
	if(str[0]=='-') k=1; /* 十进制负数 */ 
	else k=0; 
	/* 将原来的“/2”改为“/2.0”，保证当num在16~255之间，radix等于16时，也能得到正确结果 */ 
	char temp; 
	for(j=k;j<=(i-k-1)/2.0;j++) 
	{ 
		temp=str[j]; 
		str[j]=str[i-j-1]; 
		str[i-j-1]=temp; 
	} 
	return str; 
} 

int print_hex(const char* prefix, const void* data, size_t size)
{
    char  sbuf[64];
    
    int  i = 0;
    int  j = 0;
    
    if (prefix != NULL)
    {
        while ((sbuf[j] = prefix[j]) != '\0' && (j + 6) < sizeof(sbuf))
        {
            j++;
        }
    }

    for (i = 0; i < size; i++)
    {
        char  c1 = ((u8_t*)data)[i] >> 4;
        char  c2 = ((u8_t*)data)[i] & 0x0F;

        if (c1 >= 10)
        {
            c1 += 'a' - 10;
        }
        else
        {
            c1 += '0';
        }

        if (c2 >= 10)
        {
            c2 += 'a' - 10;
        }
        else
        {
            c2 += '0';
        }

        sbuf[j++] = ' ';
        sbuf[j++] = c1;
        sbuf[j++] = c2;
        
        if (((i + 1) % 16) == 0)
        {
            sbuf[j++] = '\n';
        }

        if (((i + 1) % 16) == 0 ||
            (j + 6) >= sizeof(sbuf))
        {
            sbuf[j] = '\0';
            j = 0;
            printf(sbuf);
        }
    }

    if ((i % 16) != 0)
    {
        sbuf[j++] = '\n';
    }

    if (j > 0)
    {
        sbuf[j] = '\0';
        j = 0;
        printf(sbuf);
    }

    return i;
}

