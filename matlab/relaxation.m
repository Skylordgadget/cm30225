clear; clf;
precision = 0.75;

% set initial array
arr_a = [1 1 1 1 1;
         1 8 6 4 1;
         1 1 1 7 1;
         1 2 3 7 1;
         1 1 1 1 1;];

counter = 0;

disp(arr_a);
while 1
    counter = counter + 1; % track number of iterations
    % convolve array with a matrix that selects adjacent elements
    arr_res = conv2(arr_a,[0 1 0;1 0 1;0 1 0], 'valid') ./ 4; 
    % absolute value of middle arr_a elements - arr_res elements
    diff = abs(arr_a(2:end-1, 2:end-1) - arr_res);  
    disp(diff);
    % replace arr_a middle elements with arr_res
    arr_a(2:end-1, 2:end-1) = arr_res;
    disp(arr_a);
    if diff < precision 
        break
    end 
end
disp(counter);