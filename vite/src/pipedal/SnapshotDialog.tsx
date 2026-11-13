/* eslint-disable @typescript-eslint/no-unused-vars */
/*
* MIT License
* 
* Copyright (c) Robin E.R. Davies
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
* of the Software, and to permit persons to whom the Software is furnished to do
* so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

import React from 'react';
import DialogEx from './DialogEx';
import { PiPedalModelFactory } from './PiPedalModel';
import DialogContent from '@mui/material/DialogContent';
import DialogTitle from '@mui/material/DialogTitle';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import IconButtonEx from './IconButtonEx';
import ResizeResponsiveComponent from './ResizeResponsiveComponent';
import { TransitionProps } from '@mui/material/transitions';

import Fade from '@mui/material/Fade';
import Slide from '@mui/material/Slide';
import SnapshotPanel from './SnapshotPanel';



const FadeTransition = React.forwardRef(function Transition(
    props: TransitionProps & {
        children: React.ReactElement<any, any>;
    },
    ref: React.Ref<unknown>,
) {
    return <Fade in={true} ref={ref} {...props} />;
});


const SlideTransition = React.forwardRef(function Transition(
    props: TransitionProps & {
      children: React.ReactElement<any, any>;
    },
    ref: React.Ref<unknown>,
  ) {
    return <Slide direction="up" ref={ref} {...props} />;
  });
  
  


export interface SnapshotDialogProps {
    open: boolean,
    onOk: () => void,
};

export interface SnapshotDialogState {
    fullScreen: boolean,
};

export default class SnapshotDialog extends ResizeResponsiveComponent<SnapshotDialogProps, SnapshotDialogState> {

    constructor(props: SnapshotDialogProps) {
        super(props);
        this.state = {
            fullScreen: this.getFullScreen(),
        };
    }

    componentDidMount(): void {
        super.componentDidMount();
    }

    componentWillUnmount(): void {
        super.componentWillUnmount();

    }

    getFullScreen() {
        return document.documentElement.clientHeight < 500 || document.documentElement.clientWidth < 450;
    }
    onWindowSizeChanged(width: number, height: number): void {
        this.setState(
            {
                fullScreen: this.getFullScreen(),
            }
        );
    }


    render() {
        return (
            <DialogEx fullWidth={true} tag="shapshot" open={this.props.open} onClose={() => this.props.onOk()} fullScreen={this.state.fullScreen}
                TransitionComponent={this.state.fullScreen? SlideTransition: FadeTransition} keepMounted
                style={{ userSelect: "none", }}
                PaperProps={{
                    style: {
                      
                    },
                  }}
                onEnterKey={()=>{} }
            >
                {this.state.fullScreen ? (
                <div id="snapFull" style={{ position: "fixed", left: 0, top: 0, right: 0, bottom: 0, display: "flex", flexDirection: "column", overflow: "hidden" }}>
                    <div style={{ position: "fixed", left: 0, right: 0, top: 0, height: 52, zIndex: 3000 }}>
                        <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={() => { this.props.onOk(); }} aria-label="back"
                            style={{ position: "absolute", left: 4, top: 4, zIndex: 2 }}
                        >
                            <ArrowBackIcon fontSize="small" style={{ opacity: "0.6" }} />
                        </IconButtonEx>
                        <div style={{ position: "absolute", left: 0, right: 0, top: 0, height: 52, margin: 0, display: "flex", alignItems: "center", justifyContent: "center",
                            lineHeight: 1, fontSize: 39, fontWeight: 700, color: "#FFD400",
                            whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis", padding: "0 56px" }}>
                            {PiPedalModelFactory.getInstance().pedalboard.get().name}
                        </div>
                    </div>
                    <div id="snapWrap" style={{ position: "fixed", left: 12, right: 12, top: 60, bottom: 8 }}>
                        <style>{"#snapWrap > div { padding: 0 !important; height: 100% !important; box-sizing: border-box !important; }"}</style>
                        <SnapshotPanel panelHeight="100%" />
                    </div>
                </div>
            ) : (
            <>
<DialogTitle style={{ padding: 0, height: 48, minHeight: 48, flex: "0 0 48px", position: "relative", overflow: "hidden" }}>
                    <IconButtonEx tooltip="Back" edge="start" color="inherit" onClick={() => { this.props.onOk(); }} aria-label="back"
                        style={{ position: "absolute", left: 4, top: 4, zIndex: 2 }}
                    >
                        <ArrowBackIcon fontSize="small" style={{ opacity: "0.6" }} />
                    </IconButtonEx>
                    <div style={{ textAlign: "center", lineHeight: "48px", height: 48, fontSize: 39, fontWeight: 700, color: "#FFD400",
                        whiteSpace: "nowrap", overflow: "hidden", textOverflow: "ellipsis", padding: "0 56px" }}>
                        {PiPedalModelFactory.getInstance().pedalboard.get().name}
                    </div>
                </DialogTitle>
                <DialogContent style={{
                    position: "relative", overflow: "hidden", margin: 0, padding: 0,
                    flex: this.state.fullScreen ? "1 1 0px" : undefined, minHeight: 0,
                    height: this.state.fullScreen ? undefined : "80vh",
                }}>
                    {this.state.fullScreen && (<style>{"#snapWrap > div { padding: 0 !important; box-sizing: border-box !important; height: 100% !important; }"}</style>)}
                    <div id="snapWrap" style={this.state.fullScreen
                        ? { position: "absolute", left: 12, right: 12, top: 0, bottom: 8 }
                        : { padding: "0 16px 24px 16px" }}>
                        <SnapshotPanel panelHeight={this.state.fullScreen ? "100%" : "80vh"} />
                    </div>
                </DialogContent>
            </>
            )}
            </DialogEx>
        );
    }
}