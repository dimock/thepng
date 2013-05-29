% solve test task

function img_result = img_process(thrMin, thrMax)
    if thrMax > 1
        thrMax = 1;
	end
    if thrMin < 0
        thrMin = 0;
    end
    if thrMax == 0
        thrMax = 0.01;
    end
    if thrMin >= thrMax
        thrMin = thrMax*0.99;
    end
    thrCoef = 1.1;
    in_name = 'problem_2.png';
    out_name = 'my_result.png';
    img = imread(in_name);
    img_gray = rgb2gray(img);
    img_dbl = im2double(img_gray);
    img_dbl_comp = imcomplement(img_dbl);
    % find threshold for background color
    Thr = graythresh(img_dbl)*thrCoef;
    bw_mask = im2bw(img_dbl, Thr);
    % HACK - add right part of ellipse to mask. it is still incomplete
    bw_mask = im2bw(img_dbl_comp, 0.05) .* bw_mask;
    % background mask
    % search for vertical line what ellipse starts from
    % also find complete figures mask (figures marked as black)
    [bw_mask,  y_ellipse_start] = find_ellipse(bw_mask);
    % find rectangle mask
    rect_mask = find_rect(y_ellipse_start, bw_mask);
    % exclude all figures from image and calculate average background color
    img_back = bw_mask .* img_dbl;
    bg_color = avg_color(img_back);
    % ellipse and scribble
    figures_mask = bw_mask + rect_mask;
    figures_mask = imcomplement(figures_mask);
    %denoise background
    img_denoise_bg = (ones(size(img_back)) * bg_color) .* bw_mask;
    % image of figures
    img_figures = img_dbl .* figures_mask;
    img_figures = imadjust(img_figures,[thrMin thrMax],[0 1]);
    img_figures = img_figures .* figures_mask;
    % image of rectangle
    img_rect = img_dbl .* rect_mask;
    % collect them all together
    img_result = img_denoise_bg + img_figures + img_rect;
    imwrite(img_result, out_name, 'png');
end

% does line y have at least one black point
function p = y_line_black(y, mask)
    y_line_mask = mask(y, :);
    p = prod(y_line_mask);
end

function p = x_line_black(x, ymax, mask)
    x_line_mask = mask(1:ymax, x);
    p = prod(x_line_mask);
end

% find complete ellipse mask and it's start line
function [complete_bw_mask, y_ellipse_start] = find_ellipse(bw_mask)
    W=size(bw_mask, 2);
    H=size(bw_mask, 1);
    y_ellipse_start = H;
    y_ellipse_stop = 0;
    xleft_ellipse = H;
    xright_ellipse = 0;
    segment_n = 0;
    for y=1:H
        % find horizontal line, what ellipse starts from
        if y_ellipse_start == H
            p = y_line_black( y, bw_mask);
            if segment_n == 0 && p == 0
                segment_n = 1;
            elseif segment_n == 1 && p ~= 0
                y_ellipse_start = y;
                segment_n = 2;
            end
        elseif y_ellipse_stop == 0 && segment_n > 1
            p = y_line_black( y, bw_mask);
            if p == 0 && segment_n == 2
                segment_n = 3;
            end
            if p ~= 0 && segment_n == 3
                y_ellipse_stop = y;
            end
        end
        % find left and right ellipse boundaries
        for x=1:W
            v = bw_mask(y, x);
            if y > y_ellipse_start && v == 0
                if x < xleft_ellipse
                    xleft_ellipse = x;
                elseif x > xright_ellipse
                    xright_ellipse = x;
                end
            end
        end
    end
    % make symmetrical copy of left part of the ellipse
    xcenter_ellipse = (xleft_ellipse + xright_ellipse) / 2;
    complete_bw_mask = bw_mask;
    for y=y_ellipse_start:y_ellipse_stop
        for x=1:W
            v = complete_bw_mask(y, x);
            if v == 0 && x < xcenter_ellipse
                x1 = 2*xcenter_ellipse - x;
                complete_bw_mask(y, x1) = 0;
            end
        end
    end
end

% find rectanle object mask
function rect_mask = find_rect(y_ellipse_start, bw_mask)
    % search for rectanle left coord
    xleft_rect = 0;
    segment_n = 0;
    W=size(bw_mask, 2);
    for x=1:W
        p = x_line_black(x, y_ellipse_start, bw_mask);
        if p == 0 && segment_n == 0
            segment_n = 1;
        elseif p ~= 0 && segment_n == 1
            xleft_rect = x;
            break;
        end
    end
    % collect pixels of rect mask
    rect_mask = ones(size(bw_mask));
    for y = 1:y_ellipse_start-1
        rect_mask(y, xleft_rect:W) = bw_mask(y,xleft_rect:W);
    end
    rect_mask = imcomplement(rect_mask);
end

% caclulate sum all non-zero points to find average background color
function color = avg_color(img_background)
    W=size(img_background, 2);
    H=size(img_background, 1);
    S = 0;
    num = 0;
    for y=1:H
        for x=1:W
            v = img_background(y, x);
            if v == 0
                continue
            end
            S = S + v;
            num = num + 1;
        end
    end
    color = S / num;
end
