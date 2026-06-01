// TODO: Find out the reason why the log-knob's value get corrupted when we single click it

import { useLayoutEffect, useRef, useEffect } from 'react';
import { useJuceSlider, useJuceKnob, useJuceToggle } from './hooks/juce-hooks';
import { logToLinear } from './utils/scale-transformation';
import { Button } from './components/ui/button';
import useOnClickOutside from './hooks/useOnClickOutside';
import { pull } from './WeatherAPI';

interface KnobProps {
  label: string;
  paramId: string; // parameter ID in JUCE APVTS
  min: number;
  max: number;
  initialValue?: number;
  unit: string;
  isLog?: boolean;
  decimalPlaces?: number;
  setValueRef?: React.Ref<((val: number) => void) | null>;
}

const ReadOnlySlider = ({ setValueRef, paramId }: { setValueRef?: React.Ref<((val: number) => void) | null>, paramId: string }) => {
  const { value, setValue } = useJuceSlider(paramId, 0, 100, false, 0, 0);
  if (setValueRef) {
    (setValueRef as React.MutableRefObject<((val: number) => void) | null>).current = setValue;
  }

  return (
    <div className="w-full">
      <div className="w-full h-4 bg-slate-800 rounded-full overflow-hidden">
        <div className="h-full bg-cyan-400" style={{ width: `${value}%` }} />
      </div>
      <span className="text-xs font-mono text-slate-500 mt-1">{value.toFixed(0)}%</span>
    </div>
  );
};

const Knob = ({ setValueRef, label, paramId, min, max, unit, isLog, decimalPlaces = 0, initialValue = 0 }: KnobProps) => {
  const knobRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);
  const valueRef = useRef<HTMLSpanElement>(null);
  const lastClickTimeRef = useRef<number>(0);
  const clickCountRef = useRef<number>(0);

  const {
    value,
    isEditing,
    setIsEditing,
    handleManualInput,
    onMouseDown: originalOnMouseDown,
    setValue,
  } = useJuceKnob(paramId, min, max, isLog, decimalPlaces, initialValue);

  // Handle ref assignment for external setValue access
  useEffect(() => {
    if (!setValueRef) return;

    if (typeof setValueRef === 'function') {
      // Callback ref
      setValueRef(setValue);
    } else if (setValueRef && 'current' in setValueRef) {
      // Object ref (e.g., useRef)
      (setValueRef as React.MutableRefObject<((val: number) => void) | null>).current = setValue;
    }
  }, [setValueRef, setValue]);

  // 시각적 표현을 위한 퍼센트 계산
  const percent = isLog ? logToLinear(value, min, max) : (value - min) / (max - min);
  const rotation = percent * 270 - 135;

  // Unified mouse down handler for both drag and double-click detection
  const handleKnobMouseDown = (e: React.MouseEvent) => {
    const now = Date.now();
    const timeSinceLastClick = now - lastClickTimeRef.current;

    // Double-click detection: second click within 300ms
    if (timeSinceLastClick < 300) {
      clickCountRef.current++;
      if (clickCountRef.current === 2) {
        console.log(`✏️ Double-click detected on ${paramId}, entering edit mode.`);
        setIsEditing(true);
        clickCountRef.current = 0;
        lastClickTimeRef.current = 0;
        return;
      }
    } else {
      clickCountRef.current = 1;
    }

    lastClickTimeRef.current = now;

    // If in editing mode, don't start drag
    if (isEditing) return;

    // Otherwise, start drag
    originalOnMouseDown(e);
  };

  useLayoutEffect(() => {
    if (isEditing) {
      const input = inputRef.current;
      if (input) {
        input.focus({ preventScroll: true });
        requestAnimationFrame(() => input.select());
      }
    }
  }, [isEditing]);

  // 외부 클릭 시 편집 모드 해제
  useOnClickOutside([knobRef, inputRef, valueRef], () => {  // ref list 안에 무슨 ref를 넣어야 할지 아직 확정 못 함
    if (isEditing) {
      const input = inputRef.current;
      const inputValue = input?.value ?? String(value);
      const parsed = parseFloat(inputValue);

      if (!Number.isNaN(parsed)) {
        const clamped = Math.max(min, Math.min(max, parsed));
        console.log(`✏️ Outside click for ${paramId}, value:`, inputValue, 'clamped:', clamped);
        handleManualInput({ key: 'Enter', target: { value: String(clamped) } } as any);
      } else {
        setIsEditing(false);
      }
    }
  });

  return (
    <div className="flex flex-col items-center gap-3">
      <span className="text-[10px] font-black tracking-widest text-cyan-500 uppercase select-none">{label}</span>

      <div
        ref={knobRef}
        onMouseDown={handleKnobMouseDown}
        className={`relative w-28 h-28 rounded-full bg-slate-900 shadow-[5px_5px_15px_#050505,-5px_-5px_15px_#1a1a1a] flex items-center justify-center group ${
          isEditing ? '' : 'cursor-ns-resize'
        }`}
      >
        {/* Progress Ring (SVG) */}
        <svg className="absolute z-0 w-full h-full pointer-events-none" viewBox="0 0 100 100" style={{ transform: `rotate(-225deg)` }}>
          <circle cx="50" cy="50" r="45" fill="none" stroke="#1e293b" strokeWidth="4" />
          <circle
            cx="50" cy="50" r="45" fill="none" stroke="#06b6d4" strokeWidth="4"
            strokeDasharray={`${211.5 * percent} ${282.7 - 211.5 * percent}`}
            strokeLinecap="round"
            className="transition-none drop-shadow-[0_0_5px_#06b6d4]"
          />
        </svg>

        {/* Knob Face */}
        <div className="relative z-10 w-20 h-20 rounded-full bg-gradient-to-br from-slate-800 to-slate-950 shadow-lg flex flex-col items-center justify-center">
          <div className="absolute inset-0 z-0 pointer-events-none transition-none" style={{ transform: `rotate(${rotation}deg)` }}>
            <div className="absolute top-2 left-1/2 -translate-x-1/2 w-1.5 h-1.5 bg-cyan-400 rounded-full shadow-[0_0_8px_#22d3ee]" />
          </div>

          {/* Value Text */}
          {isEditing ? (
            <input
              ref={inputRef}
              autoFocus
              type="text"
              inputMode="decimal"
              className="relative z-20 w-16 bg-transparent text-center text-white font-bold outline-none border-b border-cyan-500"
              defaultValue={value}
              onKeyDown={(e) => {
                // Only allow numbers, dots, backspace, delete, arrow keys, tab, enter
                const allowedKeys = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'Backspace', 'Delete', 'ArrowLeft', 'ArrowRight', 'Tab', 'Enter'];
                if (!allowedKeys.includes(e.key)) {
                  e.preventDefault();
                }
                // Call original handler on Enter
                if (e.key === 'Enter') {
                  console.log(`✏️ Enter pressed in input for ${paramId}, value:`, (e.target as HTMLInputElement).value);
                  handleManualInput(e);
                }
                if (e.key === 'Escape') {
                  console.log(`✏️ Escape pressed in input for ${paramId}, exiting edit mode.`);
                  setIsEditing(false);
                }
              }}
              onInput={(e) => {
                // Filter input to only numbers and single dot
                const input = e.currentTarget.value;
                const filtered = input.replace(/[^\d.]/g, '').replace(/^\./, '0.').replace(/\.(?=.*\.)/g, '');
                e.currentTarget.value = filtered;
              }}
              onFocus={(e) => {
                // Select all text on focus
                e.currentTarget.select();
              }}
            />
          ) : (
            <span
              ref={valueRef}
              className="relative z-20 text-lg font-black text-slate-100 tracking-tighter select-none"
            >
              {value}
            </span>
          )}
          <span className="relative z-20 text-[8px] font-bold text-slate-500 select-none">{unit}</span>
        </div>
      </div>
    </div>
  );
};

const KnobWrapper = ({ setValueRef, label, paramId, min, max, unit, isLog, decimalPlaces = 0, initialValue = 0 }: KnobProps) => {
  return (
    <div className="flex gap-6 items-center flex-1 flex-wrap">
      <ReadOnlySlider setValueRef={setValueRef} paramId={paramId} />
      <Knob
        label={label}
        paramId={paramId + "Mix"}
        min={min}
        max={max}
        unit={unit}
        isLog={isLog}
        decimalPlaces={decimalPlaces}
        initialValue={initialValue}
      />
    </div>
  );
};

interface BypassButtonProps {
  paramId: string;
}

function BypassButton({ paramId }: BypassButtonProps) {
  const { value: isBypassed, handleToggle } = useJuceToggle(paramId);

  return (
    <Button
      type="button"
      variant={isBypassed ? 'outline' : 'default'}
      size="sm"
      onClick={handleToggle}
      className={`h-8 w-24 px-3 text-[10px] font-black tracking-[0.2em] uppercase transition-all ${
        isBypassed
          ? 'border-slate-700 bg-slate-900 text-white hover:bg-slate-800'
          : 'border-cyan-300 bg-slate-900 text-white shadow-[0_0_12px_rgba(34,211,238,0.25)] hover:bg-slate-800'
      }`}
    >
      {isBypassed ? 'Bypass' : 'Active'}
    </Button>
  );
}


export default function LPFUI() {
  useEffect(() => {
    const interval = setInterval(async () => {
      console.log(await pull(30, 30));
    }, 1000);
    return () => clearInterval(interval);
  }, [])
  return (
    <div className="w-[640px] h-[480px] bg-black bg-[radial-gradient(circle_at_center,_#111_0%,_#000_100%)] flex flex-col items-center justify-between p-6 overflow-hidden font-sans border border-slate-800 select-none">
      <div className="w-full flex justify-between items-center border-b border-cyan-900/30 pb-2">
        <h1 className="text-2xl font-black tracking-tighter text-transparent bg-clip-text bg-gradient-to-r from-cyan-400 to-blue-600 drop-shadow-[0_0_10px_rgba(34,211,238,0.4)]">
          WeatherSound
        </h1>
        <BypassButton paramId="bypass" />
      </div>

      {/* Control Section */}
      <div className="flex gap-6 items-center flex-1 flex-wrap">
        <KnobWrapper
          label="Cloud Coverage"
          paramId="cloudCoverage"
          min={0}
          max={100}
          unit="%"
        />

        <KnobWrapper
          label="Humidity"
          paramId="humidity"
          min={0}
          max={100}
          unit="%"
        />

        <KnobWrapper
          label="Temperature"
          paramId="temperature"
          min={0}
          max={100}
          unit="%"
        />

        <KnobWrapper
          label="UV Index"
          paramId="uvIndex"
          min={0}
          max={100}
          unit="%"
        />

        <KnobWrapper
          label="Wind Speed"
          paramId="windSpeed"
          min={0}
          max={100}
          unit="%"
        />

        <KnobWrapper
          label="Wind Direction"
          paramId="windDirection"
          min={0}
          max={100}
          unit="%"
        />

        <KnobWrapper
          label="Visibility"
          paramId="visibility"
          min={0}
          max={100}
          unit="%"
        />
      </div>

      {/* Footer Decoration */}
      <div className="w-full flex justify-between text-[8px] font-mono text-slate-600 tracking-[0.3em] uppercase">
        <span>2026 ADC Japan 26</span>
        <span>THE VOLUNTEER TEAM</span>
      </div>
    </div>
  );
}