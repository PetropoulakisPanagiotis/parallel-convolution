tmp_width = width / sqrt(processes)
remain_width = width % sqrt(processes)
tmp_height = height  / sqrt(processes)
remain_height = height  % sqrt(processes)

array my_width[processes], my_height[processes], my_width_start[processes], my_height_start[processes]

for(i = 0; i < sqrt(processes); i++){
 # Find width #
 if(i < remain_width){
    my_width[i,i+1,i+2] = tmp + 1;
    my_width_start[i,i+1,i+2] = i * (tmp + 1);
    remain_width--;
  }
  else{
    my_width[i,i+1,i+2] = tmp;
    my_width_start = i * tmp;
  }
  
  # Find height #
 if(i < remain_width){
    my_width[i,i+1,i+2] = tmp + 1;
    my_width_start[i,i + (i + 1)sqrt(processes),i + (i + 2)sqrt(processes)] = i * (tmp + 1);
    remain_width--;
  }
  else{
    my_width[i,i+1,i+2] = tmp;
    my_width_start = i * tmp;
  }
}

epidi eine se omades se kathe loop briskis oria gia 3 diergasies.

to width paei an +1 eno to height ana 3 an milame gia 9 processes. 
Stin arxi xorizete to width se 3 stiles as poume kai kathe stili exi 3 processes pou exoun to idio width

